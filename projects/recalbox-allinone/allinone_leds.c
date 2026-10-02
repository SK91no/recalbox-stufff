// SPDX-License-Identifier: GPL-2.0-only
/*
 * WorldSemi WS2812B individually-addressable LED driver using SPI
 *
 * Copyright 2022 Chuanhong Guo <gch981213@gmail.com>
 * Copyright (C) 2024 digitalLumberjack at recalbox.com.
 *
 * This driver simulates WS2812B protocol using SPI MOSI pin. A one pulse
 * is transferred as 3'b110 and a zero pulse is 3'b100. For this driver to
 * work properly, the SPI frequency should be 2.105MHz~2.85MHz and it needs
 * to transfer all the bytes continuously.
 */

#include <linux/led-class-multicolor.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <linux/mutex.h>
#include <linux/of.h>

#define WS2812B_BYTES_PER_COLOR 3
#define WS2812B_NUM_COLORS 3
/* A continuous 0 for 50us+ as the 'reset' signal */
#define WS2812B_RESET_LEN 90

struct ws2812b_led {
  struct led_classdev_mc mc_cdev;
  struct mc_subled subled[WS2812B_NUM_COLORS];
  int cascade;
};

struct ws2812b_priv {
  struct led_classdev ldev;
  struct spi_device *spi;
  struct mutex mutex;
  int num_leds;
  size_t data_len;
  u8 *data_buf;
  struct ws2812b_led leds[];
};

/**
 * ws2812b_set_byte - convert a byte of data to 3-byte SPI data for pulses
 * @priv: pointer to the private data structure
 * @offset: offset of the target byte in the data stream
 * @val: 1-byte data to be set
 *
 * WS2812B receives a stream of bytes from DI, takes the first 3 byte as LED
 * brightness and pases the rest to the next LED through the DO pin.
 * This function assembles a single byte of data to the LED:
 * A bit is represented with a pulse of specific length. A long pulse is a 1
 * and a short pulse is a 0.
 * SPI transfers data continuously, MSB first. We can send 3'b100 to create a
 * 0 pulse and 3'b110 for a 1 pulse. In this way, a byte of data takes up 3
 * bytes in a SPI transfer:
 *  1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0
 * Let's rearrange it in 8 bits:
 *  1x01x01x 01x01x01 x01x01x0
 * The higher 3 bits, middle 2 bits and lower 3 bits are represented with the
 * 1st, 2nd and 3rd byte in the SPI transfer respectively.
 * There are only 8 combinations for 3 bits and 4 for 2 bits, so we can create
 * a lookup table for the 3 bytes.
 * e.g. For 0x6b -> 2'b01101011:
 *  Bit 7-5: 3'b011 -> 10011011 -> 0x9b
 *  Bit 4-3: 2'b01  -> 01001101 -> 0x4d
 *  Bit 2-0: 3'b011 -> 00110110 -> 0x36
 */
static void ws2812b_set_byte(struct ws2812b_priv *priv, size_t offset, u8 val)
{
  /* The lookup table for Bit 7-5 4-3 2-0 */
  const u8 h3b[] = { 0x92, 0x93, 0x9a, 0x9b, 0xd2, 0xd3, 0xda, 0xdb };
  const u8 m2b[] = { 0x49, 0x4d, 0x69, 0x6d };
  const u8 l3b[] = { 0x24, 0x26, 0x34, 0x36, 0xa4, 0xa6, 0xb4, 0xb6 };
  u8 *p = priv->data_buf + WS2812B_RESET_LEN + (offset * WS2812B_BYTES_PER_COLOR);

  p[0] = h3b[val >> 5]; /* Bit 7-5 */
  p[1] = m2b[(val >> 3) & 0x3]; /* Bit 4-3 */
  p[2] = l3b[val & 0x7]; /* Bit 2-0 */
}

static int ws2812b_set(struct led_classdev *cdev,
                       enum led_brightness brightness)
{
  struct led_classdev_mc *mc_cdev = lcdev_to_mccdev(cdev);
  struct ws2812b_led *led =
  container_of(mc_cdev, struct ws2812b_led, mc_cdev);
  struct ws2812b_priv *priv = dev_get_drvdata(cdev->dev->parent);
  int ret;
  int i;

  led_mc_calc_color_components(mc_cdev, brightness);

  mutex_lock(&priv->mutex);
  for (i = 0; i < WS2812B_NUM_COLORS; i++)
    ws2812b_set_byte(priv, led->cascade * WS2812B_NUM_COLORS + i,
                     led->subled[i].brightness);
  ret = spi_write(priv->spi, priv->data_buf, priv->data_len);
  mutex_unlock(&priv->mutex);

  return ret;
}
static u32 color_idx[WS2812B_NUM_COLORS] = {
  LED_COLOR_ID_RED,
  LED_COLOR_ID_GREEN,
  LED_COLOR_ID_BLUE,
};

static int ws2812b_prepare_led(struct device *dev, struct ws2812b_priv *priv, char * name, int cur_led)
{
  int i;
  priv->leds[cur_led].mc_cdev.subled_info = priv->leds[cur_led].subled;
  priv->leds[cur_led].mc_cdev.led_cdev.name = name;
  priv->leds[cur_led].mc_cdev.num_colors = WS2812B_NUM_COLORS;
  priv->leds[cur_led].mc_cdev.led_cdev.max_brightness = 255;
  priv->leds[cur_led].mc_cdev.led_cdev.brightness = 255;
  priv->leds[cur_led].mc_cdev.led_cdev.brightness_set_blocking = ws2812b_set;

  for (i = 0; i < WS2812B_NUM_COLORS; i++) {
    priv->leds[cur_led].subled[i].color_index = color_idx[i];
    priv->leds[cur_led].subled[i].intensity = 255;
  }

  priv->leds[cur_led].cascade = cur_led;

  return led_classdev_multicolor_register_ext(dev, &priv->leds[cur_led].mc_cdev, NULL);
}


static int ws2812b_probe(struct spi_device *spi)
{
  struct device *dev = &spi->dev;
  int cur_led = 0;
  struct ws2812b_priv *priv;
  int num_leds, i, cnt, ret;

  int player = 1;
  int buttons = 6;
  int start = 1;
  int select = 1;
  int hotkey = 1;

  of_property_read_u32(dev->of_node, "player", &player);

  // Enable GPIO
  struct gpio_desc * enable = devm_gpiod_get_index_optional(dev, "aio_controls", 0, GPIOD_OUT_LOW);
  if(enable)
    gpiod_set_value(enable, 1);

/*  struct device_node *controller_node = of_find_node_by_name(NULL, player ? "aio_jammactrl_0" : "aio_jammactrl_1");

  if(of_property_read_u32(controller_node, "rgb_buttons", &buttons)){
    // Get controller configuration if no
    of_property_read_u32(controller_node, "buttons", &buttons);
  }*/
  of_property_read_u32(dev->of_node, "rgb_start", &start);
  of_property_read_u32(dev->of_node, "rgb_select", &select);
  of_property_read_u32(dev->of_node, "rgb_hotkey", &hotkey);
  dev_info(dev, "Config for player %d, buttons: %d, start: %d, select: %d, hotkey: %d", player, buttons, start, select, hotkey);

  num_leds = (buttons + start + select + hotkey) * 2;
  if(player == 2) {
    num_leds += 2;
  }

  priv = devm_kzalloc(dev, struct_size(priv, leds, num_leds), GFP_KERNEL);
  if (!priv)
    return -ENOMEM;
  priv->data_len =
    num_leds * WS2812B_BYTES_PER_COLOR * WS2812B_NUM_COLORS +
    WS2812B_RESET_LEN;
  priv->data_buf = kzalloc(priv->data_len, GFP_KERNEL);
  if (!priv->data_buf)
    return -ENOMEM;

  for (i = 0; i < num_leds * WS2812B_NUM_COLORS; i++)
    ws2812b_set_byte(priv, i, 0);

  mutex_init(&priv->mutex);
  priv->num_leds = num_leds;
  priv->spi = spi;


  // Player 2 has Status and Test before buttons
  if(player == 2){
    cur_led = 0;
    ret = ws2812b_prepare_led(dev, priv, "aio_status", cur_led);

    if (ret) {
      dev_err(dev, "registration of aio_status failed.");
      goto ERR_UNREG_LEDS;
    }
    cur_led = 1;
    ret = ws2812b_prepare_led(dev, priv, "aio_test", cur_led);
    if (ret) {
      dev_err(dev, "registration of aio_test failed.");
      goto ERR_UNREG_LEDS;
    }
    cur_led = 2;
  }

  char name[256];
  int map6[] = {1, 2, 3, 6, 5, 4};
  int map8[] = {1, 2, 3, 7, 8, 6, 5, 4};
  int * mappingbtn;
  if(buttons > 8) buttons = 8;
  if(buttons == 8) mappingbtn = map8;
  else mappingbtn = map6;
  for(int button = 0; button < buttons; button++){
    for(int i = 1; i <= 2; i++){
      sprintf(name, "aio_p%d_b%d_%d", player, mappingbtn[button], i);
      ret = ws2812b_prepare_led(dev, priv, name, cur_led);
      cur_led++;
      if (ret) {
        dev_err(dev, "registration of %s failed.", name);
        goto ERR_UNREG_LEDS;
      }
    }
  }
  if(select){
    for(int i = 1; i <= 2; i++)
    {
      sprintf(name, "aio_p%d_start_%d", player, i);
      ret = ws2812b_prepare_led(dev, priv, name, cur_led);
      cur_led++;
      if (ret)
      {
        dev_err(dev, "registration of %s failed.", name);
        goto ERR_UNREG_LEDS;
      }
    }
  }
  if(start){
    for(int i = 1; i <= 2; i++)
    {
      sprintf(name, "aio_p%d_select_%d", player, i);
      ret = ws2812b_prepare_led(dev, priv, name, cur_led);
      cur_led++;
      if (ret)
      {
        dev_err(dev, "registration of %s failed.", name);
        goto ERR_UNREG_LEDS;
      }
    }
  }
  if(player == 1 && hotkey){
    for(int i = 1; i <= 2; i++)
    {
      sprintf(name, "aio_hotkey_%d", i);
      ret = ws2812b_prepare_led(dev, priv, name, cur_led);
      cur_led++;
      if (ret)
      {
        dev_err(dev, "registration of %s failed.", name);
        goto ERR_UNREG_LEDS;
      }
    }
  }

  spi_set_drvdata(spi, priv);

  return 0;
  ERR_UNREG_LEDS:
  for (; cur_led >= 0; cur_led--)
    led_classdev_multicolor_unregister(&priv->leds[cur_led].mc_cdev);
  mutex_destroy(&priv->mutex);
  kfree(priv->data_buf);
  return ret;
}

static void ws2812b_remove(struct spi_device *spi)
{
  struct ws2812b_priv *priv = spi_get_drvdata(spi);
  int cur_led;

  for (cur_led = priv->num_leds - 1; cur_led >= 0; cur_led--)
    led_classdev_multicolor_unregister(&priv->leds[cur_led].mc_cdev);
  kfree(priv->data_buf);
  mutex_destroy(&priv->mutex);
}

static const struct spi_device_id ws2812b_spi_ids[] = {
  { "allinoneleds" },
  {},
};
MODULE_DEVICE_TABLE(spi, ws2812b_spi_ids);

static const struct of_device_id ws2812b_dt_ids[] = {
  { .compatible = "allinoneleds" },
  {},
};
MODULE_DEVICE_TABLE(of, ws2812b_dt_ids);

static struct spi_driver ws2812b_driver = {
  .probe		= ws2812b_probe,
  .remove		= ws2812b_remove,
  .id_table	= ws2812b_spi_ids,
  .driver = {
    .name		= KBUILD_MODNAME,
    .of_match_table	= ws2812b_dt_ids,
  },
};

module_spi_driver(ws2812b_driver);

MODULE_AUTHOR("Chuanhong Guo <gch981213@gmail.com>, digitalLumberjack <digitalLumberjack@recalbox.com>");
MODULE_DESCRIPTION("WS2812B LED driver using SPI - adapted for Recalbox AllInOne");
MODULE_LICENSE("GPL");
