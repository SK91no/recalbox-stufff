// SPDX-License-Identifier: GPL-2.0-only
/*
 *  PCA953x 4/8/16/24/40 bit I/O ports
 *
 *  Copyright (C) 2005 Ben Gardner <bgardner@wabtec.com>
 *  Copyright (C) 2007 Marvell International Ltd.
 *  Copyright (C) 2023 digitalLumberjack at recalbox.com.
 *
 *  Derived from drivers/gpio/gpio-pca953x.c
 */

#include <linux/acpi.h>
#include <linux/bitmap.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/platform_data/pca953x.h>

#include <sound/soc.h>

#include <linux/unaligned.h>

#include <linux/leds.h>

// Interupts
#include <linux/dmi.h>

#define PCA953X_INPUT 0x00
#define PCA953X_OUTPUT 0x01
#define PCA953X_INVERT 0x02
#define PCA953X_DIRECTION 0x03

#define REG_ADDR_MASK GENMASK(5, 0)
#define REG_ADDR_EXT BIT(6)
#define REG_ADDR_AI BIT(7)

#define PCAL953X_OUT_STRENGTH 0x20
#define PCAL953X_IN_LATCH 0x22
#define PCAL953X_PULL_EN 0x23
#define PCAL953X_PULL_SEL 0x24
#define PCAL953X_INT_MASK 0x25
#define PCAL953X_INT_STAT 0x26
#define PCAL953X_OUT_CONF 0x27

#define PCA_GPIO_MASK GENMASK(7, 0)

#define PCAL_GPIO_MASK GENMASK(4, 0)
#define PCAL_PINCTRL_MASK GENMASK(6, 5)

#define PCA_INT BIT(8)
#define PCA_PCAL BIT(9)
#define PCA_LATCH_INT (PCA_PCAL | PCA_INT)
#define PCA953X_TYPE BIT(12)
#define PCA_TYPE_MASK GENMASK(15, 12)

#define PCA_CHIP_TYPE(x) ((x) &PCA_TYPE_MASK)

#define DEBUG 1

static const struct i2c_device_id pca953x_id[] = {
    {"allinone", 16 | PCA953X_TYPE | PCA_INT},
    {}};
MODULE_DEVICE_TABLE(i2c, pca953x_id);


static const struct acpi_gpio_params pca953x_irq_gpios = {0, 0, true};

static const struct acpi_gpio_mapping pca953x_acpi_irq_gpios[] = {
    {"irq-gpios", &pca953x_irq_gpios, 1, ACPI_GPIO_QUIRK_ABSOLUTE_NUMBER},
    {}};

static int pca953x_acpi_get_irq(struct device *dev) {
  int ret;

  ret = devm_acpi_dev_add_driver_gpios(dev, pca953x_acpi_irq_gpios);
  if (ret)
    dev_warn(dev, "can't add GPIO ACPI mapping\n");

  ret = acpi_dev_gpio_irq_get_by(ACPI_COMPANION(dev), "irq-gpios", 0);
  if (ret < 0) {
    dev_info(dev, "could not get acpi gpio by irq-gpios\n");
    return ret;
  }

  dev_info(dev, "ACPI interrupt quirk (IRQ %d)\n", ret);
  return ret;
}

static const struct acpi_device_id pca953x_acpi_ids[] = {
    {"INT3491", 16 | PCA953X_TYPE | PCA_LATCH_INT},
    {}};

#define MAX_BANK 5
#define BANK_SZ 8
#define MAX_LINE (MAX_BANK * BANK_SZ)

#define NBANK(chip) DIV_ROUND_UP(chip->gpio_chip.ngpio, BANK_SZ)

struct pca953x_reg_config {
  int direction;
  int output;
  int input;
  int invert;
};

static const struct pca953x_reg_config pca953x_regs = {
    .direction = PCA953X_DIRECTION,
    .output = PCA953X_OUTPUT,
    .input = PCA953X_INPUT,
    .invert = PCA953X_INVERT,
};

struct pca953x_chip {
  unsigned gpio_start;
  struct mutex i2c_lock;
  struct regmap *regmap;

  struct mutex irq_lock;
  DECLARE_BITMAP(irq_mask, MAX_LINE);
  DECLARE_BITMAP(irq_stat, MAX_LINE);
  DECLARE_BITMAP(irq_trig_raise, MAX_LINE);
  DECLARE_BITMAP(irq_trig_fall, MAX_LINE);
  struct irq_chip irq_chip;

  atomic_t wakeup_path;

  struct i2c_client *client;
  struct gpio_chip gpio_chip;
  const char *const *names;
  unsigned long driver_data;
  struct regulator *regulator;

  const struct pca953x_reg_config *regs;
};

static int pca953x_bank_shift(struct pca953x_chip *chip) {
  return fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
}

#define PCA953x_BANK_INPUT BIT(0)
#define PCA953x_BANK_OUTPUT BIT(1)
#define PCA953x_BANK_POLARITY BIT(2)
#define PCA953x_BANK_CONFIG BIT(3)

#define PCAL9xxx_BANK_IN_LATCH BIT(8 + 2)
#define PCAL9xxx_BANK_PULL_EN BIT(8 + 3)
#define PCAL9xxx_BANK_PULL_SEL BIT(8 + 4)
#define PCAL9xxx_BANK_IRQ_MASK BIT(8 + 5)
#define PCAL9xxx_BANK_IRQ_STAT BIT(8 + 6)

/*
 * We care about the following registers:
 * - Standard set, below 0x40, each port can be replicated up to 8 times
 *   - PCA953x standard
 *     Input port			0x00 + 0 * bank_size	R
 *     Output port			0x00 + 1 * bank_size	RW
 *     Polarity Inversion port		0x00 + 2 * bank_size	RW
 *     Configuration port		0x00 + 3 * bank_size	RW
 *   - PCA957x with mixed up registers
 *     Input port			0x00 + 0 * bank_size	R
 *     Polarity Inversion port		0x00 + 1 * bank_size	RW
 *     Bus hold port			0x00 + 2 * bank_size	RW
 *     Configuration port		0x00 + 4 * bank_size	RW
 *     Output port			0x00 + 5 * bank_size	RW
 *
 * - Extended set, above 0x40, often chip specific.
 *   - PCAL6524/PCAL9555A with custom PCAL IRQ handling:
 *     Input latch register		0x40 + 2 * bank_size	RW
 *     Pull-up/pull-down enable reg	0x40 + 3 * bank_size    RW
 *     Pull-up/pull-down select reg	0x40 + 4 * bank_size    RW
 *     Interrupt mask register		0x40 + 5 * bank_size	RW
 *     Interrupt status register	0x40 + 6 * bank_size	R
 *
 * - Registers with bit 0x80 set, the AI bit
 *   The bit is cleared and the registers fall into one of the
 *   categories above.
 */

static bool pca953x_check_register(struct pca953x_chip *chip, unsigned int reg,
                                   u32 checkbank) {
  int bank_shift = pca953x_bank_shift(chip);
  int bank = (reg & REG_ADDR_MASK) >> bank_shift;
  int offset = reg & (BIT(bank_shift) - 1);

  /* Special PCAL extended register check. */
  if (reg & REG_ADDR_EXT) {
    if (!(chip->driver_data & PCA_PCAL))
      return false;
    bank += 8;
  }

  /* Register is not in the matching bank. */
  if (!(BIT(bank) & checkbank))
    return false;

  /* Register is not within allowed range of bank. */
  if (offset >= NBANK(chip))
    return false;

  return true;
}

static bool pca953x_readable_register(struct device *dev, unsigned int reg) {
  struct pca953x_chip *chip = dev_get_drvdata(dev);
  u32 bank;

  bank = PCA953x_BANK_INPUT | PCA953x_BANK_OUTPUT |
         PCA953x_BANK_POLARITY | PCA953x_BANK_CONFIG;

  if (chip->driver_data & PCA_PCAL) {
    bank |= PCAL9xxx_BANK_IN_LATCH | PCAL9xxx_BANK_PULL_EN |
            PCAL9xxx_BANK_PULL_SEL | PCAL9xxx_BANK_IRQ_MASK |
            PCAL9xxx_BANK_IRQ_STAT;
  }

  return pca953x_check_register(chip, reg, bank);
}

static bool pca953x_writeable_register(struct device *dev, unsigned int reg) {
  struct pca953x_chip *chip = dev_get_drvdata(dev);
  u32 bank;

  bank = PCA953x_BANK_OUTPUT | PCA953x_BANK_POLARITY |
         PCA953x_BANK_CONFIG;

  if (chip->driver_data & PCA_PCAL)
    bank |= PCAL9xxx_BANK_IN_LATCH | PCAL9xxx_BANK_PULL_EN |
            PCAL9xxx_BANK_PULL_SEL | PCAL9xxx_BANK_IRQ_MASK;

  return pca953x_check_register(chip, reg, bank);
}

static bool pca953x_volatile_register(struct device *dev, unsigned int reg) {
  struct pca953x_chip *chip = dev_get_drvdata(dev);
  u32 bank;

  bank = PCA953x_BANK_INPUT;

  if (chip->driver_data & PCA_PCAL)
    bank |= PCAL9xxx_BANK_IRQ_STAT;

  return pca953x_check_register(chip, reg, bank);
}

static const struct regmap_config pca953x_i2c_regmap = {
    .reg_bits = 8,
    .val_bits = 8,

    .use_single_read = true,
    .use_single_write = true,

    .readable_reg = pca953x_readable_register,
    .writeable_reg = pca953x_writeable_register,
    .volatile_reg = pca953x_volatile_register,

    .disable_locking = true,
    .cache_type = REGCACHE_RBTREE,
    .max_register = 0x7f,
};


static u8 pca953x_recalc_addr(struct pca953x_chip *chip, int reg, int off) {
  int bank_shift = pca953x_bank_shift(chip);
  int addr = (reg & PCAL_GPIO_MASK) << bank_shift;
  int pinctrl = (reg & PCAL_PINCTRL_MASK) << 1;
  u8 regaddr = pinctrl | addr | (off / BANK_SZ);

  return regaddr;
}

static int pca953x_write_regs(struct pca953x_chip *chip, int reg, unsigned long *val) {
  u8 regaddr = pca953x_recalc_addr(chip, reg, 0);
  u8 value[MAX_BANK];
  int i, ret;

  for (i = 0; i < NBANK(chip); i++)
    value[i] = bitmap_get_value8(val, i * BANK_SZ);

  ret = regmap_bulk_write(chip->regmap, regaddr, value, NBANK(chip));
  if (ret < 0) {
    dev_err(&chip->client->dev, "failed writing register\n");
    return ret;
  }

  return 0;
}

static int pca953x_read_regs(struct pca953x_chip *chip, int reg, unsigned long *val) {
  u8 regaddr = pca953x_recalc_addr(chip, reg, 0);
  u8 value[MAX_BANK];
  int i, ret;

  ret = regmap_bulk_read(chip->regmap, regaddr, value, NBANK(chip));
  if (ret < 0) {
    dev_err(&chip->client->dev, "failed reading register\n");
    return ret;
  }

  for (i = 0; i < NBANK(chip); i++)
    bitmap_set_value8(val, value[i], i * BANK_SZ);

  return 0;
}

static int pca953x_gpio_direction_input(struct gpio_chip *gc, unsigned off) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  u8 dirreg = pca953x_recalc_addr(chip, chip->regs->direction, off);
  u8 bit = BIT(off % BANK_SZ);
  int ret;

  mutex_lock(&chip->i2c_lock);
  ret = regmap_write_bits(chip->regmap, dirreg, bit, bit);
  mutex_unlock(&chip->i2c_lock);
  return ret;
}

static int pca953x_gpio_direction_output(struct gpio_chip *gc,
                                         unsigned off, int val) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  u8 dirreg = pca953x_recalc_addr(chip, chip->regs->direction, off);
  u8 outreg = pca953x_recalc_addr(chip, chip->regs->output, off);
  u8 bit = BIT(off % BANK_SZ);
  int ret;

  mutex_lock(&chip->i2c_lock);
  /* set output level */
  ret = regmap_write_bits(chip->regmap, outreg, bit, val ? bit : 0);
  if (ret)
    goto exit;

  /* then direction */
  ret = regmap_write_bits(chip->regmap, dirreg, bit, 0);
exit:
  mutex_unlock(&chip->i2c_lock);
  return ret;
}

static int pca953x_gpio_get_value(struct gpio_chip *gc, unsigned off) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  u8 inreg = pca953x_recalc_addr(chip, chip->regs->input, off);
  u8 bit = BIT(off % BANK_SZ);
  u32 reg_val;
  int ret;

  mutex_lock(&chip->i2c_lock);
  ret = regmap_read(chip->regmap, inreg, &reg_val);
  mutex_unlock(&chip->i2c_lock);
  if (ret < 0)
    return ret;

  return !!(reg_val & bit);
}

static void pca953x_gpio_set_value(struct gpio_chip *gc, unsigned off, int val) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  u8 outreg = pca953x_recalc_addr(chip, chip->regs->output, off);
  u8 bit = BIT(off % BANK_SZ);

  mutex_lock(&chip->i2c_lock);
  regmap_write_bits(chip->regmap, outreg, bit, val ? bit : 0);
  mutex_unlock(&chip->i2c_lock);
}

static int pca953x_gpio_get_direction(struct gpio_chip *gc, unsigned off) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  u8 dirreg = pca953x_recalc_addr(chip, chip->regs->direction, off);
  u8 bit = BIT(off % BANK_SZ);
  u32 reg_val;
  int ret;

  mutex_lock(&chip->i2c_lock);
  ret = regmap_read(chip->regmap, dirreg, &reg_val);
  mutex_unlock(&chip->i2c_lock);
  if (ret < 0)
    return ret;

  if (reg_val & bit)
    return GPIO_LINE_DIRECTION_IN;

  return GPIO_LINE_DIRECTION_OUT;
}

static int pca953x_gpio_get_multiple(struct gpio_chip *gc,
                                     unsigned long *mask, unsigned long *bits) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  DECLARE_BITMAP(reg_val, MAX_LINE);
  int ret;

  mutex_lock(&chip->i2c_lock);
  ret = pca953x_read_regs(chip, chip->regs->input, reg_val);
  mutex_unlock(&chip->i2c_lock);
  if (ret)
    return ret;

  bitmap_replace(bits, bits, reg_val, mask, gc->ngpio);
  return 0;
}

static void pca953x_gpio_set_multiple(struct gpio_chip *gc,
                                      unsigned long *mask, unsigned long *bits) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  DECLARE_BITMAP(reg_val, MAX_LINE);
  int ret;

  mutex_lock(&chip->i2c_lock);
  ret = pca953x_read_regs(chip, chip->regs->output, reg_val);
  if (ret)
    goto exit;

  bitmap_replace(reg_val, reg_val, bits, mask, gc->ngpio);

  pca953x_write_regs(chip, chip->regs->output, reg_val);
exit:
  mutex_unlock(&chip->i2c_lock);
}

static int pca953x_gpio_set_pull_up_down(struct pca953x_chip *chip,
                                         unsigned int offset,
                                         unsigned long config) {
  u8 pull_en_reg = pca953x_recalc_addr(chip, PCAL953X_PULL_EN, offset);
  u8 pull_sel_reg = pca953x_recalc_addr(chip, PCAL953X_PULL_SEL, offset);
  u8 bit = BIT(offset % BANK_SZ);
  int ret;

  /*
   * pull-up/pull-down configuration requires PCAL extended
   * registers
   */
  if (!(chip->driver_data & PCA_PCAL))
    return -ENOTSUPP;

  mutex_lock(&chip->i2c_lock);

  /* Configure pull-up/pull-down */
  if (config == PIN_CONFIG_BIAS_PULL_UP)
    ret = regmap_write_bits(chip->regmap, pull_sel_reg, bit, bit);
  else if (config == PIN_CONFIG_BIAS_PULL_DOWN)
    ret = regmap_write_bits(chip->regmap, pull_sel_reg, bit, 0);
  else
    ret = 0;
  if (ret)
    goto exit;

  /* Disable/Enable pull-up/pull-down */
  if (config == PIN_CONFIG_BIAS_DISABLE)
    ret = regmap_write_bits(chip->regmap, pull_en_reg, bit, 0);
  else
    ret = regmap_write_bits(chip->regmap, pull_en_reg, bit, bit);

exit:
  mutex_unlock(&chip->i2c_lock);
  return ret;
}

static int pca953x_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
                                   unsigned long config) {
  struct pca953x_chip *chip = gpiochip_get_data(gc);

  switch (pinconf_to_config_param(config)) {
    case PIN_CONFIG_BIAS_PULL_UP:
    case PIN_CONFIG_BIAS_PULL_PIN_DEFAULT:
    case PIN_CONFIG_BIAS_PULL_DOWN:
    case PIN_CONFIG_BIAS_DISABLE:
      return pca953x_gpio_set_pull_up_down(chip, offset, config);
    default:
      return -ENOTSUPP;
  }
}

static void pca953x_setup_gpio(struct pca953x_chip *chip, int gpios) {
  struct gpio_chip *gc;

  gc = &chip->gpio_chip;

  gc->direction_input = pca953x_gpio_direction_input;
  gc->direction_output = pca953x_gpio_direction_output;
  gc->get = pca953x_gpio_get_value;
  gc->set = pca953x_gpio_set_value;
  gc->get_direction = pca953x_gpio_get_direction;
  gc->get_multiple = pca953x_gpio_get_multiple;
  gc->set_multiple = pca953x_gpio_set_multiple;
  gc->set_config = pca953x_gpio_set_config;
  gc->can_sleep = true;

  gc->base = chip->gpio_start;
  gc->ngpio = gpios;
  gc->label = dev_name(&chip->client->dev);
  gc->parent = &chip->client->dev;
  gc->owner = THIS_MODULE;
  gc->names = chip->names;
}

static void pca953x_irq_mask(struct irq_data *d) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  irq_hw_number_t hwirq = irqd_to_hwirq(d);

  clear_bit(hwirq, chip->irq_mask);
}

static void pca953x_irq_unmask(struct irq_data *d) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  irq_hw_number_t hwirq = irqd_to_hwirq(d);

  set_bit(hwirq, chip->irq_mask);
}

static int pca953x_irq_set_wake(struct irq_data *d, unsigned int on) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);

  if (on)
    atomic_inc(&chip->wakeup_path);
  else
    atomic_dec(&chip->wakeup_path);

  return irq_set_irq_wake(chip->client->irq, on);
}

static void pca953x_irq_bus_lock(struct irq_data *d) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);

  mutex_lock(&chip->irq_lock);
}

static void pca953x_irq_bus_sync_unlock(struct irq_data *d) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  DECLARE_BITMAP(irq_mask, MAX_LINE);
  DECLARE_BITMAP(reg_direction, MAX_LINE);
  int level;

  if (chip->driver_data & PCA_PCAL) {
    /* Enable latch on interrupt-enabled inputs */
    pca953x_write_regs(chip, PCAL953X_IN_LATCH, chip->irq_mask);

    bitmap_complement(irq_mask, chip->irq_mask, gc->ngpio);

    /* Unmask enabled interrupts */
    pca953x_write_regs(chip, PCAL953X_INT_MASK, irq_mask);
  }

  /* Switch direction to input if needed */
  pca953x_read_regs(chip, chip->regs->direction, reg_direction);

  bitmap_or(irq_mask, chip->irq_trig_fall, chip->irq_trig_raise, gc->ngpio);
  bitmap_complement(reg_direction, reg_direction, gc->ngpio);
  bitmap_and(irq_mask, irq_mask, reg_direction, gc->ngpio);

  /* Look for any newly setup interrupt */
  for_each_set_bit(level, irq_mask, gc->ngpio)
      pca953x_gpio_direction_input(&chip->gpio_chip, level);

  mutex_unlock(&chip->irq_lock);
}

static int pca953x_irq_set_type(struct irq_data *d, unsigned int type) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  irq_hw_number_t hwirq = irqd_to_hwirq(d);

  if (!(type & IRQ_TYPE_EDGE_BOTH)) {
    dev_err(&chip->client->dev, "irq %d: unsupported type %d\n",
            d->irq, type);
    return -EINVAL;
  }

  assign_bit(hwirq, chip->irq_trig_fall, type & IRQ_TYPE_EDGE_FALLING);
  assign_bit(hwirq, chip->irq_trig_raise, type & IRQ_TYPE_EDGE_RISING);

  return 0;
}

static void pca953x_irq_shutdown(struct irq_data *d) {
  struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
  struct pca953x_chip *chip = gpiochip_get_data(gc);
  irq_hw_number_t hwirq = irqd_to_hwirq(d);

  clear_bit(hwirq, chip->irq_trig_raise);
  clear_bit(hwirq, chip->irq_trig_fall);
}

static bool pca953x_irq_pending(struct pca953x_chip *chip, unsigned long *pending) {
  struct gpio_chip *gc = &chip->gpio_chip;
  DECLARE_BITMAP(reg_direction, MAX_LINE);
  DECLARE_BITMAP(old_stat, MAX_LINE);
  DECLARE_BITMAP(cur_stat, MAX_LINE);
  DECLARE_BITMAP(new_stat, MAX_LINE);
  DECLARE_BITMAP(trigger, MAX_LINE);
  int ret;

  if (chip->driver_data & PCA_PCAL) {
    /* Read the current interrupt status from the device */
    ret = pca953x_read_regs(chip, PCAL953X_INT_STAT, trigger);
    if (ret)
      return false;

    /* Check latched inputs and clear interrupt status */
    ret = pca953x_read_regs(chip, chip->regs->input, cur_stat);
    if (ret)
      return false;

    /* Apply filter for rising/falling edge selection */
    bitmap_replace(new_stat, chip->irq_trig_fall, chip->irq_trig_raise, cur_stat, gc->ngpio);

    bitmap_and(pending, new_stat, trigger, gc->ngpio);

    return !bitmap_empty(pending, gc->ngpio);
  }

  ret = pca953x_read_regs(chip, chip->regs->input, cur_stat);
  if (ret)
    return false;

  /* Remove output pins from the equation */
  pca953x_read_regs(chip, chip->regs->direction, reg_direction);

  bitmap_copy(old_stat, chip->irq_stat, gc->ngpio);

  bitmap_and(new_stat, cur_stat, reg_direction, gc->ngpio);
  bitmap_xor(cur_stat, new_stat, old_stat, gc->ngpio);
  bitmap_and(trigger, cur_stat, chip->irq_mask, gc->ngpio);

  bitmap_copy(chip->irq_stat, new_stat, gc->ngpio);

  if (bitmap_empty(trigger, gc->ngpio))
    return false;

  bitmap_and(cur_stat, chip->irq_trig_fall, old_stat, gc->ngpio);
  bitmap_and(old_stat, chip->irq_trig_raise, new_stat, gc->ngpio);
  bitmap_or(new_stat, old_stat, cur_stat, gc->ngpio);
  bitmap_and(pending, new_stat, trigger, gc->ngpio);

  return !bitmap_empty(pending, gc->ngpio);
}

static int process_inputs(struct gpio_chip *gpio_chip);

static struct config {
  struct gpio_chip *gpio_chip_0;
  struct gpio_chip *gpio_chip_1;
  struct task_struct *config_thread;
  struct task_struct *debounce_thread;
  struct mutex process_mutex;
  uint hk_on_start;
  uint credit_on_start_btn1;
  uint exit_on_start;
  uint sound_on_start;
  uint autofire;
  uint autofire_time;
  uint amp_boost;
  uint amp_disable;
  uint dual_joy;
  uint debounce_time_ms;
  uint player_count;
  //Specific all in one
  struct gpio_desc *enableDAC;
  struct gpio_desc *disableAMP;
  struct gpio_desc *util;
  struct gpio_desc *gain0;
  struct gpio_desc *gain1;
  uint p1_credits_also_p2;
  uint p1_buttons;
  uint p2_buttons;
} allinone_config;

static irqreturn_t pca953x_irq_handler(int irq, void *devid) {

  struct pca953x_chip *chip = (struct pca953x_chip *) devid;
  if (&chip->gpio_chip == allinone_config.gpio_chip_0) {
    DEBUG &&printk(KERN_INFO
                   "allinone: IRQ triggered on chip 0\n");
    process_inputs(allinone_config.gpio_chip_0);
  } else if (&chip->gpio_chip == allinone_config.gpio_chip_1) {
    DEBUG &&printk(KERN_INFO
                   "allinone: IRQ triggered on chip 1\n");
    process_inputs(allinone_config.gpio_chip_1);
  }

  return IRQ_RETVAL(1);
}

static int pca953x_irq_setup(struct pca953x_chip *chip, int irq_base) {
  struct i2c_client *client = chip->client;
  struct irq_chip *irq_chip = &chip->irq_chip;
  DECLARE_BITMAP(reg_direction, MAX_LINE);
  DECLARE_BITMAP(irq_stat, MAX_LINE);
  struct gpio_irq_chip *girq;
  int ret;

  printk(KERN_INFO
         "allinone: setting IRQ\n");

  if (!client->irq)
    return 0;

  if (irq_base == -1)
    return 0;

  if (!(chip->driver_data & PCA_INT))
    return 0;

  ret = pca953x_read_regs(chip, chip->regs->input, irq_stat);
  if (ret)
    return ret;

  /*
   * There is no way to know which GPIO line generated the
   * interrupt.  We have to rely on the previous read for
   * this purpose.
   */
  pca953x_read_regs(chip, chip->regs->direction, reg_direction);
  bitmap_and(chip->irq_stat, irq_stat, reg_direction, chip->gpio_chip.ngpio);
  mutex_init(&chip->irq_lock);

  irq_chip->name = dev_name(&client->dev);
  irq_chip->irq_mask = pca953x_irq_mask;
  irq_chip->irq_unmask = pca953x_irq_unmask;
  irq_chip->irq_set_wake = pca953x_irq_set_wake;
  irq_chip->irq_bus_lock = pca953x_irq_bus_lock;
  irq_chip->irq_bus_sync_unlock = pca953x_irq_bus_sync_unlock;
  irq_chip->irq_set_type = pca953x_irq_set_type;
  irq_chip->irq_shutdown = pca953x_irq_shutdown;

  girq = &chip->gpio_chip.irq;
  girq->chip = irq_chip;
  /* This will let us handle the parent IRQ in the driver */
  girq->parent_handler = NULL;
  girq->num_parents = 0;
  girq->parents = NULL;
  girq->default_type = IRQ_TYPE_NONE;
  girq->handler = handle_simple_irq;
  girq->threaded = true;
  girq->first = irq_base; /* FIXME: get rid of this */

  ret = devm_request_threaded_irq(&client->dev, client->irq,
                                  NULL, pca953x_irq_handler,
                                  IRQF_ONESHOT | IRQF_SHARED,
                                  dev_name(&client->dev), chip);
  if (ret) {
    dev_err(&client->dev, "failed to request irq %d\n",
            client->irq);
    return ret;
  }

  return 0;
}

static int device_pca95xx_init(struct pca953x_chip *chip) {
  DECLARE_BITMAP(val, MAX_LINE);
  u8 regaddr;
  int ret;

  regaddr = pca953x_recalc_addr(chip, chip->regs->output, 0);
  ret = regcache_sync_region(chip->regmap, regaddr,
                             regaddr + NBANK(chip) - 1);
  if (ret)
    goto out;

  regaddr = pca953x_recalc_addr(chip, chip->regs->direction, 0);
  ret = regcache_sync_region(chip->regmap, regaddr,
                             regaddr + NBANK(chip) - 1);
  if (ret)
    goto out;

  bitmap_zero(val, MAX_LINE);

  ret = pca953x_write_regs(chip, chip->regs->invert, val);
out:
  return ret;
}

/* EXPANDER */

/* CONTROLS
 *
 * Chip 0 : address = 0x20
 *                                    __________
 *   P1-UP               IO0-0  <--- |0        8| ---> IO1-0
 *   P1-DOWN             IO0-1  <--- |1        9| ---> IO1-1  HOTKEY
 *   P1-RIGHT            IO0-2  <--- |2       10| ---> IO1-2  P1-B7
 *   P1-LEFT             IO0-3  <--- |3       11| ---> IO1-3  P1-B4
 *   P1-B8               IO0-4  <--- |4       12| ---> IO1-4  P1-B3
 *   P1-START            IO0-5  <--- |5       13| ---> IO1-5  P1-B2
 *   P1-SELECT           IO0-6  <--- |6       14| ---> IO1-6  P1-B5
 *   P1-B6               IO0-7  <--- |7       15| ---> IO1-7  P1-B1
 *                                    ‾‾‾‾‾‾‾‾‾‾
 *
 * Chip 1 : address = 0x21
 *                                    __________
 *                       IO0-0  <--- |16      24| ---> IO1-0  P2-B1
 *   P2-B8               IO0-1  <--- |17      25| ---> IO1-1  P2-SELECT
 *   P2-B2               IO0-2  <--- |18      26| ---> IO1-2  P2-LEFT
 *   P2-B6               IO0-3  <--- |19      27| ---> IO1-3  P2-RIGHT
 *   P2-B3               IO0-4  <--- |20      28| ---> IO1-4  P2-DOWN
 *   P2-B5               IO0-5  <--- |21      29| ---> IO1-5  P2-UP
 *   P2-B7               IO0-6  <--- |22      30| ---> IO1-6  P2-START
 *   P2-B4               IO0-7  <--- |23      31| ---> IO1-7
 *                                    ‾‾‾‾‾‾‾‾‾‾
 *
 */

#define MAX_PLAYERS 2

struct input_dev *player_devs[MAX_PLAYERS];

// Standard buttons mapping
#define BTN_PER_PLAYER 13
#define BTN_PER_PLAYER_ON_ALLINONE 11
#define TOTAL_GPIO_ON_PCA 32

// Keep in mind the vanilla mapping for Recalbox RGB Jamma is (SNES notation):
// B A Y
// X L R
// We use this mapping to get B and A on first line, whatever the number of buttons
// Keep also in mind that the A is SOUTH and B is EAST (they are switched) on Linux notation, and that NORTH and WEST are also reversed in auto mapping...
//
static const unsigned int buttons_codes[BTN_PER_PLAYER] = {
    BTN_A,         //  ALLINONE_BTN_1
    BTN_B,         //  ALLINONE_BTN_2
    BTN_X,         //  ALLINONE_BTN_3
    BTN_Y,         //  ALLINONE_BTN_4
    BTN_TL,        //  ALLINONE_BTN_5
    BTN_TR,        //  ALLINONE_BTN_6
    BTN_TL2,       //  ALLINONE_BTN_7
    BTN_TR2,       //  ALLINONE_BTN_8
    BTN_START,     //  ALLINONE_BTN_START
    BTN_SELECT,    //  ALLINONE_BTN_SELECT
    BTN_MODE,      //  ALLINONE_BTN_HOTKEY
    KEY_VOLUMEUP,  //  VOLUME_UP
    KEY_VOLUMEDOWN,//  VOLUME_DOWN
};

#define ALLINONE_BTN_1 0
#define ALLINONE_BTN_2 1
#define ALLINONE_BTN_3 2
#define ALLINONE_BTN_4 3
#define ALLINONE_BTN_5 4
#define ALLINONE_BTN_6 5
#define ALLINONE_BTN_7 6
#define ALLINONE_BTN_8 7
#define ALLINONE_BTN_START 8
#define ALLINONE_BTN_COIN 9
#define BTN_HOTKEY 10
#define VOLUME_UP 11
#define VOLUME_DOWN 12

// Games buttons are from 0 to 5 in buttons codes
#define LAST_GAME_BUTTON 7


#define PLAYER1 0
#define PLAYER2 1

static int buttons_bits[MAX_PLAYERS][BTN_PER_PLAYER_ON_ALLINONE] = {
    // Player 1
    // The value is the bit of the data containing the
    // information for the button at the same index in buttons_codes array
    // Start is a special case
    {15, 13, 12, 11, 14, 7, 10, 4, -1, 6, 9},
    // Player 2
    {24, 18, 20, 23, 21, 19, 22, 17, 30, 25, -1},
};

#define P1_START 5

#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

static const int direction_bits[MAX_PLAYERS][4] = {
    // Player 1
    {0, 1, 3, 2},
    {29, 28, 26, 27},
};


// Logic is not the same for each buttons (coins and service/test are reversed)
// TODO: add service and test for next version
static unsigned short buttonsReleasedValues[TOTAL_GPIO_ON_PCA] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1};


#define PRESSED(data, btn) ((btn) != -1 ? (((data >> (btn)) & 1) ^ (buttonsReleasedValues[btn])) : 0)
#define PRESSED_CODE(data, player, btn) (PRESSED(data, buttons_bits[player][btn]))

#define WAIT_SYNC() usleep_range(50000, 80000);
//Exit delay (HK + START) set to 2 secondes
#define EXIT_DELAY(now, start) ((now - start) / 1000000000UL >= 2)
//HK delay set to 1 second
#define HOTKEY_DELAY(now, start) ((now - start) / 1000000000UL >= 1)
static long long int last_start_press = 0;
static unsigned long start_state = 0;
static unsigned int should_release_start = 1;
static short can_exit = 0;
static short should_release_hk = 0;

#define PRESS_AND_SYNC(player, button)                  \
  input_report_key(player_devs[(player)], (button), 1); \
  input_sync(player_devs[(player)]);                    \
  WAIT_SYNC();
#define RELEASE_AND_SYNC(player, button)                \
  input_report_key(player_devs[(player)], (button), 0); \
  input_sync(player_devs[(player)]);                    \
  WAIT_SYNC();
#define PRESS_AND_RELEASE(player, button) \
  PRESS_AND_SYNC(player, button);         \
  RELEASE_AND_SYNC(player, button);
#define HOTKEY_PATTERNS_ENABLED(allinone_config) (allinone_config.credit_on_start_btn1 || allinone_config.hk_on_start || allinone_config.exit_on_start || allinone_config.sound_on_start)

static long long press_time[MAX_PLAYERS][BTN_PER_PLAYER_ON_ALLINONE] = {0};
static bool turbo_enabled[MAX_PLAYERS][BTN_PER_PLAYER_ON_ALLINONE] = {0};
static long long turbo_time[MAX_PLAYERS][BTN_PER_PLAYER_ON_ALLINONE] = {0};
static bool turbo_state[MAX_PLAYERS][BTN_PER_PLAYER_ON_ALLINONE] = {0};


void manage_special_inputs(unsigned long long *data_chips, long long int *time_ns) {
  // Special case for START pattern:
  // Simple press + release of START -> START event
  // Simple press of START + BTN1 -> send SELECT (COIN) on release
  // Simple press of START + an other button -> send HOTKEY + BUTTON
  // Simple press of START + an other button 3sec -> auto fire on button
  // Long press (3s) of START -> send HOTKEY + START
  if (PRESSED(*data_chips, P1_START)) {
    DEBUG &&printk(KERN_INFO "allinone: START is pressed\n");
    if (last_start_press == 0) {
      // First press
      DEBUG &&printk(KERN_INFO "allinone: saving START first press\n");
      last_start_press = *time_ns;
      start_state = *data_chips;
      should_release_start = 1;
      should_release_hk = 0;
      can_exit = allinone_config.exit_on_start;
    } else if (start_state != *data_chips) {
      // Start is pressed and the state has changed since last loop
      // Only player 1
      int player = PLAYER1;
      for (int button = 0; button < BTN_PER_PLAYER_ON_ALLINONE; button++) {
        if (press_time[player][button] == 0 && PRESSED_CODE(*data_chips, player, button)) {
          // Saving press time
          press_time[player][button] = *time_ns;
        } else if (press_time[player][button] > 0 && !PRESSED_CODE(*data_chips, player, button)) {
          // button released while start pressed, we must act
          // Checking if simple press (time < 3sec)
          if (*time_ns - press_time[player][button] < 3000000000LL) {
            // Credit ?
            if (allinone_config.credit_on_start_btn1) {
              if (button == ALLINONE_BTN_1) {
                printk(KERN_INFO "allinone: credit (SELECT) triggered (START+BTN1) for player=%d\n", player);
                PRESS_AND_RELEASE(player, BTN_SELECT);
                can_exit = 0;
                should_release_start = 0;
              }
            }
            // Hotkey + BTN ?
            // Check if we should use START + BTN pattern (Hotkey)
            // Only if not disabled in config, and credit not already used, and this is the first hk event
            if (allinone_config.hk_on_start && should_release_start && !should_release_hk) {
              if (player == PLAYER1) {
                // As another button has been pressed, we press hotkey before
                printk(KERN_INFO "allinone: sending HK + BTN%d\n", button);
                PRESS_AND_SYNC(PLAYER1, BTN_MODE);
                can_exit = 0;
                should_release_hk = 1;
              }
            }
          } else {
            // AUTO FIRE because press time > 3 sec
            if (allinone_config.autofire && should_release_start && !should_release_hk) {
              printk(KERN_INFO "allinone: setting autofire for player %d on + BTN%d to %d\n", player, button, !turbo_enabled[player][button]);
              turbo_enabled[player][button] = !turbo_enabled[player][button];
              can_exit = 0;
            }
          }
          press_time[player][button] = 0;
        }
      }
      // Directions for volume
      if (allinone_config.sound_on_start) {
        for (int direction = DIR_UP; direction <= DIR_DOWN; direction++) {
          if (PRESSED(*data_chips, direction_bits[player][direction])) {
            // Volume (UP/DOWN)
            if (direction == DIR_UP) {
              printk(KERN_INFO "allinone: VOLUME UP\n");
              PRESS_AND_RELEASE(player, buttons_codes[VOLUME_UP]);
              should_release_start = 0;
            } else if (direction == DIR_DOWN) {
              printk(KERN_INFO "allinone: VOLUME DOWN\n");
              PRESS_AND_RELEASE(player, buttons_codes[VOLUME_DOWN]);
              should_release_start = 0;
            }
          }
        }
      } else if (allinone_config.hk_on_start && !should_release_hk) {
        // Directions up down if not volume on start enabled but hk on start is enabled
        for (int direction = DIR_UP; direction <= DIR_DOWN; direction++) {
          if (PRESSED(*data_chips, direction_bits[player][direction])) {
            printk(KERN_INFO
                   "allinone: sending HK + DIRECTION %d\n",
                   direction);
            PRESS_AND_SYNC(PLAYER1, BTN_MODE);
            can_exit = 0;
            should_release_hk = 1;
          }
        }
      }

      // Directions left/right if hk_on_start enabled
      if (!should_release_hk) {
        for (int direction = DIR_LEFT; direction <= DIR_RIGHT; direction++) {
          if (PRESSED(*data_chips, direction_bits[player][direction])) {
            // As another direction has been pressed, we press hotkey before
            printk(KERN_INFO "allinone: sending HK + DIRECTION %d\n", direction);
            PRESS_AND_SYNC(PLAYER1, BTN_MODE);
            can_exit = 0;
            should_release_hk = 1;
          }
        }
      }
      start_state = *data_chips;
    }
  } else {
    // Start is not pressed, let's check if we have to act
    if (last_start_press != 0) {
      // Start released
      if (should_release_start) {
        DEBUG &&printk(KERN_INFO
                       "allinone: start released\n");
        if (should_release_hk) {
          DEBUG &&printk(KERN_INFO
                         "allinone: release hotkey\n");
          should_release_hk = 0;
          RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
        } else if (can_exit && EXIT_DELAY(*time_ns, last_start_press)) {
          // Longest press, so we send both HK + START
          DEBUG &&printk(KERN_INFO
                         "allinone: long press : sending HK + START\n");
          PRESS_AND_SYNC(PLAYER1, BTN_MODE);
          PRESS_AND_SYNC(PLAYER1, BTN_START);
          RELEASE_AND_SYNC(PLAYER1, BTN_START);
          RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
        } else if (HOTKEY_DELAY(*time_ns, last_start_press)) {
          // Long press, so we send HK
          DEBUG &&printk(KERN_INFO
                         "allinone: middle press : sending HK\n");
          PRESS_AND_RELEASE(PLAYER1, BTN_MODE);
        } else {
          // Simple start
          DEBUG &&printk(KERN_INFO
                         "allinone: quick press: sending START\n");
          PRESS_AND_RELEASE(PLAYER1, BTN_START);
        }
      } else if (should_release_hk) {
        // Start have been released after a credit
        RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
      }
    }
    should_release_start = 1;
    last_start_press = 0;
  }
  // For other players, only credits and turbos
  for (int player = 1; player < allinone_config.player_count; player++) {
    if (PRESSED(*data_chips, buttons_bits[player][ALLINONE_BTN_START])) {
      for (int button = 0; button < LAST_GAME_BUTTON; button++) {
        if (press_time[player][button] == 0 && PRESSED_CODE(*data_chips, player, button)) {
          // Saving press time
          press_time[player][button] = *time_ns;
        } else if (press_time[player][button] > 0 && !PRESSED_CODE(*data_chips, player, button)) {
          // button released while start pressed, we must act
          // Checking if simple press (time < 3sec)
          if (allinone_config.credit_on_start_btn1 && *time_ns - press_time[player][button] < 3000000000LL) {
            // Credit ?
            if (button == ALLINONE_BTN_1) {
              printk(KERN_INFO "allinone: credit (SELECT) triggered (START+BTN1) for player=%d\n", player);
              PRESS_AND_RELEASE(player, BTN_SELECT);
            }
          } else if (allinone_config.autofire && *time_ns - press_time[player][button] >= 3000000000LL) {
            // AUTO FIRE because press time > 3 sec
            printk(KERN_INFO "allinone: setting autofire for player %d on + BTN%d to %d\n", player, button, !turbo_enabled[player][button]);
            turbo_enabled[player][button] = !turbo_enabled[player][button];
          }
          press_time[player][button] = 0;
        }
      }
    }
    if (allinone_config.p1_credits_also_p2 && PRESSED_CODE(*data_chips, PLAYER1, ALLINONE_BTN_COIN)) {
      *data_chips &= ~(1UL << buttons_bits[PLAYER2][ALLINONE_BTN_COIN]);
      printk(KERN_INFO "allinone: coin 1 pressed, pressing also coin 2.");
    }
  }
}

/**
 *
 * @param data_chips the data read from both chips
 * The right 16 bits are for the chip 2, middle 16 bits for the chip 1, left 16 bits for the chip 0
 */
static void input_report(u_int64_t *data_chips, long long int *time_ns) {
  int player = 0, buttonIndex = 0, buttonValue = 0;

  /*
  if (HOTKEY_PATTERNS_ENABLED(allinone_config)) {
    buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_START] = -1;
    manage_special_inputs(data_chips, time_ns);
  } else {
    buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_START] = P1_START;
  }*/
  // Always seek special inputs as HK + LEFT RIGHT is always enabled (for clones)
  buttons_bits[PLAYER1][ALLINONE_BTN_START] = -1;
  manage_special_inputs(data_chips, time_ns);

  for (player = 0; player < allinone_config.player_count; player++) {
    // Only process P1 if start + credit is not running
    if (player > PLAYER1 || should_release_start) {
      if (allinone_config.dual_joy) {
        if (player == PLAYER2) {
          input_report_abs(player_devs[PLAYER1], ABS_RY, PRESSED(*data_chips, direction_bits[player][DIR_DOWN]) - PRESSED(*data_chips, direction_bits[player][DIR_UP]));
          input_report_abs(player_devs[PLAYER1], ABS_RX, PRESSED(*data_chips, direction_bits[player][DIR_RIGHT]) - PRESSED(*data_chips, direction_bits[player][DIR_LEFT]));
        } else {
          input_report_abs(player_devs[player], ABS_Y, PRESSED(*data_chips, direction_bits[player][DIR_DOWN]) - PRESSED(*data_chips, direction_bits[player][DIR_UP]));
          input_report_abs(player_devs[player], ABS_X, PRESSED(*data_chips, direction_bits[player][DIR_RIGHT]) - PRESSED(*data_chips, direction_bits[player][DIR_LEFT]));
        }
      } else {
        input_report_abs(player_devs[player], ABS_Y, PRESSED(*data_chips, direction_bits[player][DIR_DOWN]) - PRESSED(*data_chips, direction_bits[player][DIR_UP]));
        input_report_abs(player_devs[player], ABS_X, PRESSED(*data_chips, direction_bits[player][DIR_RIGHT]) - PRESSED(*data_chips, direction_bits[player][DIR_LEFT]));
      }

      for (buttonIndex = 0; buttonIndex < BTN_PER_PLAYER_ON_ALLINONE; buttonIndex++) {
        buttonValue = PRESSED_CODE(*data_chips, player, buttonIndex);
        input_report_key(player_devs[player], buttons_codes[buttonIndex], buttonValue);
      }
      printk(KERN_INFO "allinone: input sync\n");
      input_sync(player_devs[player]);
    }
  }
}

static u_int64_t lastGpioState = 0;
static u_int64_t debounceGpioValue = 0;
static long long int lastActionOn[TOTAL_GPIO_ON_PCA] = {0};

#define SETGPIO(data_addr, gpio, value) (*(data_addr) = ((*(data_addr) & (~(1ULL << (gpio)))) | ((value) << (gpio))))

static int debounce(u_int64_t *data_chips, u_int64_t *debounced_data, long long int *time_ns) {
  u_int64_t gpioVal;
  *debounced_data = *data_chips;
  unsigned int gpio;
  for (gpio = 0; gpio < TOTAL_GPIO_ON_PCA; gpio++) {
    if (lastActionOn[gpio] != 0) {
      // We are in debounce time lap
      // We cancel this event by setting the saved value at debounce start
      SETGPIO(debounced_data, gpio, (debounceGpioValue >> gpio) & 1);
    } else {
      gpioVal = (((*data_chips) >> gpio) & 1);
      if (((lastGpioState >> gpio) & 1) != gpioVal) {
        // Gpio changed its state since last event
        // We set the start time for debounce and the value of the gpio
        lastActionOn[gpio] = *time_ns;
        // Saving the value that will be still during the debounce
        SETGPIO(&debounceGpioValue, gpio, gpioVal);
      }
    }
  }
  lastGpioState = *data_chips;
  return 0;
}
static const unsigned long mask = 0xFFFF;

static int read_gpios(u_int64_t *gpio_data) {
  unsigned long gpio_values0 = 0xFFFF;
  unsigned long gpio_values1 = 0xFFFF;
  u_int64_t gpio_values2 = 0xFFFF;
  if (allinone_config.gpio_chip_0 != NULL) {
    if (pca953x_gpio_get_multiple(allinone_config.gpio_chip_0, &mask, &gpio_values0)) {
      printk(KERN_INFO
             "allinone: unable to read gpio on chip 0, skipping\n");
      return -1;
    };
  }
  if (allinone_config.gpio_chip_1 != NULL) {
    if (pca953x_gpio_get_multiple(allinone_config.gpio_chip_1, &mask, &gpio_values1)) {
      printk(KERN_INFO
             "allinone: unable to read gpio chip 1, skipping\n");
      return -1;
    }
  }
  *gpio_data = (gpio_values1 << 16) | gpio_values0;
  return 0;
}

static int read_gpios_one_chip(u_int64_t *gpio_data, struct gpio_chip *gpio_chip) {
  if (pca953x_gpio_get_multiple(gpio_chip, &mask, (unsigned long *) gpio_data)) {
    printk(KERN_INFO
           "allinone: unable to read gpio on chip %s, skipping\n",
           gpio_chip == allinone_config.gpio_chip_0 ? "0" : "1");
    return -1;
  };
  return 0;
}

static u_int64_t gpio_data = 0xFFFFFFFFFFFF;

static int process_inputs(struct gpio_chip *gpio_chip) {
  u_int64_t debounced_data = 0;
  u_int64_t gpio_data_chip = 0;
  long long int time_start = ktime_to_ns(ktime_get_boottime());
  DEBUG &&printk(KERN_INFO "allinone: locking mutex\n");
  mutex_lock(&allinone_config.process_mutex);
  for (int i = PLAYER1; i < allinone_config.player_count; i++) {
    if (player_devs[i] == NULL) {
      printk(KERN_INFO "allinone: some players are NULL, unable to process inputs\n");
      mutex_unlock(&allinone_config.process_mutex);
      return 0;
    }
  }
  if (gpio_chip != NULL) {
    if (read_gpios_one_chip(&gpio_data_chip, gpio_chip)) {
      DEBUG &&printk(KERN_INFO "allinone: unable to read gpio chip, unlocking mutex\n");
      mutex_unlock(&allinone_config.process_mutex);
      return -1;
    }
    DEBUG &&printk(KERN_INFO "allinone: read gpio values on chip : %llu\n", gpio_data_chip);
    if (gpio_chip == allinone_config.gpio_chip_0) {
      gpio_data = ((gpio_data & 0xFFFF0000) | gpio_data_chip);
    } else if (gpio_chip == allinone_config.gpio_chip_1) {
      gpio_data = ((gpio_data & 0x0000FFFF) | (gpio_data_chip << 16));
    }
    DEBUG &&printk(KERN_INFO "allinone: gpio_data = %llu\n", gpio_data);
  } else {
    if (read_gpios(&gpio_data)) {
      printk(KERN_INFO "allinone: unable to read gpio chips in batch, unlocking mutext\n");
      mutex_unlock(&allinone_config.process_mutex);
      return -1;
    }
  }
  debounce(&gpio_data, &debounced_data, &time_start);
  input_report(&debounced_data, &time_start);
  mutex_unlock(&allinone_config.process_mutex);
  return 0;
}

static int process_debounce_and_turbo(void *idx) {
  unsigned int button;
  long long int time_ns;

  bool must_process_input;
  bool must_send_turbo;

  while (!kthread_should_stop()) {
    time_ns = ktime_to_ns(ktime_get_boottime());
    must_process_input = false;
    for (button = 0; button < TOTAL_GPIO_ON_PCA; button++) {
      if (lastActionOn[button] != 0) {
        if (time_ns - lastActionOn[button] > (1000000ul * allinone_config.debounce_time_ms)) {
          // This gpio must be debounced
          DEBUG &&printk(KERN_INFO
                         "allinone: debouncing thread: finish debouncing of %d\n",
                         button);
          must_process_input = 1;
          lastActionOn[button] = 0;
        }
      }
    }
    if (allinone_config.autofire) {
      for (int player = 0; player < allinone_config.player_count; player++) {
        if ((player == PLAYER1 && PRESSED(gpio_data, P1_START)) || PRESSED(gpio_data, buttons_bits[player][ALLINONE_BTN_START])) {
          // Using turbo only if start is not pressed
          continue;
        }
        must_send_turbo = false;
        for (button = 0; button < BTN_PER_PLAYER_ON_ALLINONE; button++) {
          if (turbo_enabled[player][button] && PRESSED(gpio_data, buttons_bits[player][button]) && time_ns - turbo_time[player][button] > allinone_config.autofire_time) {
            turbo_state[player][button] = !turbo_state[player][button];
            turbo_time[player][button] = time_ns;
            /*printk(KERN_INFO
                   "allinone: turbo mode, setting state of button %d to %d\n",
                   button, turbo_state[player][button]);*/
            input_report_key(player_devs[player], buttons_codes[button], turbo_state[player][button]);
            must_send_turbo = true;
          }
        }
        if (must_send_turbo) {
          input_sync(player_devs[player]);
        }
      }
    }


    if (must_process_input) {
      process_inputs(NULL);
    }
    usleep_range(2000, 4000);
  }
  return 0;
}

static int unregister_controllers(void) {
  int player;
  for (player = 0; player < MAX_PLAYERS; player++) {
    if (player_devs[player] != NULL) {
      input_unregister_device(player_devs[player]);
      player_devs[player] = NULL;
    }
  }
  return 0;
}

static const char *controllers_names[MAX_PLAYERS] = {"AllInOneP1", "AllInOneP2"};

static int register_controllers(void) {
  int err = 0, i = 0, player = 0;
  for (player = 0; player < allinone_config.player_count; player++) {
    if (!(player_devs[player] = input_allocate_device())) {
      pr_err("allinone: not enough memory for input device\n");
      return -ENOMEM;
    }
    printk(KERN_INFO "allinone: register player %d controller\n", player);

    player_devs[player]->name = controllers_names[player];
    player_devs[player]->phys = controllers_names[player];
    player_devs[player]->id.bustype = BUS_I2C;
    player_devs[player]->id.vendor = 0x7262;
    player_devs[player]->id.product = 0x0A02;
    player_devs[player]->id.version = 0x0100;

    player_devs[player]->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

    for (i = 0; i < 2; i++) {
      input_set_abs_params(player_devs[player], ABS_X + i, -1, 1, 0, 0);
      input_set_abs_params(player_devs[player], ABS_RX + i, -1, 1, 0, 0);
    }
    for (i = 0; i < BTN_PER_PLAYER; i++)
      __set_bit(buttons_codes[i], player_devs[player]->keybit);

    if ((err = input_register_device(player_devs[player]))) {
      pr_err("allinone: Cannot register input device\n");
      input_free_device(player_devs[player]);
      player_devs[player] = NULL;
      return -1;
    }
  }
  return 0;
}
// Load configuration from recalbox-crt-options.cfg
// Used to get Hotkey mode
#define READ_SIZE_MAX 2048
#define LINE_SIZE_MAX 256
static char read_buf[READ_SIZE_MAX];
static const char *config_path = "/boot/crt/recalbox-crt-options.cfg";

static int load_config(void) {
  struct file *fp = NULL;
  ssize_t read_size = 0;
  size_t cursor = 0;
  char line[LINE_SIZE_MAX];
  char optionname[LINE_SIZE_MAX];
  int optionvalue = 0;
  size_t line_start = 0;
  size_t line_len = 0;
  int scanret = 0;

  fp = filp_open(config_path, O_RDONLY, 0);
  if (IS_ERR(fp) || !fp) {
    //printk(KERN_INFO "allinone: config file not found, skipping configuration loading\n");
    return 0;
  }

  read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
  if (read_size <= 0) {
    filp_close(fp, NULL);
    //printk(KERN_INFO "allinone: empty config file found, skipping configuration loading\n");
    return 0;
  }
  filp_close(fp, NULL);
  for (cursor = 0; cursor < read_size; cursor++) {
    line[cursor - line_start] = read_buf[cursor];
    line_len++;
    if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0' || cursor == read_size - 1) {
      if (line_len > 1 && line[0] != '#') {
        if (line[line_len - 1] == '\n')
          line[line_len - 1] = '\0';
        scanret = sscanf(line, "%s = %d", optionname, &optionvalue);
        if (scanret == 2) {
          if (strcmp(optionname, "options.jamma.controls.hk_on_start") == 0) {
            if (allinone_config.hk_on_start != optionvalue) {
              printk(KERN_INFO "allinone: switch hk_on_start to %d\n", optionvalue);
              allinone_config.hk_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.sound_on_start") == 0) {
            if (allinone_config.sound_on_start != optionvalue) {
              printk(KERN_INFO "allinone: switch sound_on_start to %d\n", optionvalue);
              allinone_config.sound_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.credit_on_start_btn1") == 0) {
            if (allinone_config.credit_on_start_btn1 != optionvalue) {
              printk(KERN_INFO "allinone: switch credit_on_start_btn1 to %d\n", optionvalue);
              allinone_config.credit_on_start_btn1 = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.exit_on_start") == 0) {
            if (allinone_config.exit_on_start != optionvalue) {
              printk(KERN_INFO "allinone: switch exit_on_start to %d\n", optionvalue);
              allinone_config.exit_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.autofire") == 0) {
            if (allinone_config.autofire != optionvalue) {
              printk(KERN_INFO "allinone: switch autofire to %d\n", optionvalue);
              allinone_config.autofire = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.autofire_ms") == 0) {
            if (allinone_config.autofire_time != optionvalue * 1000000) {
              printk(KERN_INFO "allinone: switch autofire_ms to %d\n", optionvalue * 1000000);
              allinone_config.autofire_time = optionvalue * 1000000;
            }
          } else if (strcmp(optionname, "options.jamma.controls.4players") == 0) {
            if ((allinone_config.player_count == 2 && optionvalue == 1) || (allinone_config.player_count == 4 && optionvalue == 0)) {
              printk(KERN_INFO "allinone: switch player_count to %d\n", (optionvalue == 1 ? 4 : 2));
              mutex_lock(&allinone_config.process_mutex);
              unregister_controllers();
              allinone_config.player_count = (optionvalue == 1 ? 4 : 2);
              register_controllers();
              mutex_unlock(&allinone_config.process_mutex);
            }
          } else if (strcmp(optionname, "options.jamma.controls.dualjoysticks") == 0) {
            if (allinone_config.dual_joy != optionvalue) {
              printk(KERN_INFO "allinone: switch dual_joy to %d\n", optionvalue);
              allinone_config.dual_joy = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.debounce_time_ms") == 0) {
            if (allinone_config.debounce_time_ms != optionvalue) {
              printk(KERN_INFO "allinone: switch debounce_time_ms to %d\n", optionvalue);
              allinone_config.debounce_time_ms = optionvalue;
            }
          }
        }
      }
      line_start += line_len;
      line_len = 0;
      memset(line, 0, 128);
    }
  }
  return 0;
}

static int watch_configuration(void *idx) {
  int gpio_value = 0;
  while (!kthread_should_stop()) {
    // Read file
    load_config();
    usleep_range(2000000, 5000000);
  }
  return 0;
}

static int pca953x_probe(struct i2c_client *client) {

  struct pca953x_platform_data *pdata;
  struct pca953x_chip *chip;
  int irq_base = 0;
  int ret;
  int gpio_idx = 0;
  struct regulator *reg;
  const struct regmap_config *regmap_config;

  chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
  if (chip == NULL)
    return -ENOMEM;

  pdata = dev_get_platdata(&client->dev);
  if (pdata) {
    irq_base = pdata->irq_base;
    chip->gpio_start = pdata->gpio_base;
  } else {
    struct gpio_desc *reset_gpio;

    chip->gpio_start = -1;
    irq_base = 0;

    /*
     * See if we need to de-assert a reset pin.
     *
     * There is no known ACPI-enabled platforms that are
     * using "reset" GPIO. Otherwise any of those platform
     * must use _DSD method with corresponding property.
     */
    reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
                                         GPIOD_OUT_LOW);
    if (IS_ERR(reset_gpio))
      return PTR_ERR(reset_gpio);
  }

  chip->client = client;

  reg = devm_regulator_get(&client->dev, "vcc");
  if (IS_ERR(reg))
    return dev_err_probe(&client->dev, PTR_ERR(reg), "reg get err\n");

  ret = regulator_enable(reg);
  if (ret) {
    dev_err(&client->dev, "reg en err: %d\n", ret);
    return ret;
  }
  chip->regulator = reg;

  const void *match;

  match = device_get_match_data(&client->dev);
  if (!match) {
    ret = -ENODEV;
    goto err_exit;
  }

  chip->driver_data = (uintptr_t) match;

  i2c_set_clientdata(client, chip);

  pca953x_setup_gpio(chip, chip->driver_data & PCA_GPIO_MASK);

  regmap_config = &pca953x_i2c_regmap;

  chip->regmap = devm_regmap_init_i2c(client, regmap_config);
  if (IS_ERR(chip->regmap)) {
    ret = PTR_ERR(chip->regmap);
    goto err_exit;
  }

  regcache_mark_dirty(chip->regmap);

  mutex_init(&chip->i2c_lock);
  /*
   * In case we have an i2c-mux controlled by a GPIO provided by an
   * expander using the same driver higher on the device tree, read the
   * i2c adapter nesting depth and use the retrieved value as lockdep
   * subclass for chip->i2c_lock.
   *
   * REVISIT: This solution is not complete. It protects us from lockdep
   * false positives when the expander controlling the i2c-mux is on
   * a different level on the device tree, but not when it's on the same
   * level on a different branch (in which case the subclass number
   * would be the same).
   *
   * TODO: Once a correct solution is developed, a similar fix should be
   * applied to all other i2c-controlled GPIO expanders (and potentially
   * regmap-i2c).
   */
  lockdep_set_subclass(&chip->i2c_lock,
                       i2c_adapter_depth(client->adapter));

  /* initialize cached registers from their original values.
   * we can't share this chip with another i2c master.
   */

  chip->regs = &pca953x_regs;
  ret = device_pca95xx_init(chip);
  if (ret)
    goto err_exit;

  ret = pca953x_irq_setup(chip, irq_base);
  if (ret)
    goto err_exit;

  ret = devm_gpiochip_add_data(&client->dev, &chip->gpio_chip, chip);
  if (ret)
    goto err_exit;


  // We are a on control chip
  for (gpio_idx = 0; gpio_idx < 16; gpio_idx++) {
    if (pca953x_gpio_direction_input(&chip->gpio_chip, gpio_idx) != 0) {
      dev_err(&client->dev, "cannot set input for gpio %d on chip %d\n", gpio_idx, client->addr);
      goto err_exit;
    }
  }

  dev_info(&client->dev, "created allinone pca at %d\n", client->addr);
  if (client->addr == 0x20) {
    dev_info(&client->dev, "chip 1\n");
    allinone_config.gpio_chip_0 = &chip->gpio_chip;
    if (process_inputs(&chip->gpio_chip) == 0) {
      if ((gpio_data & 0xFFFF) != 0xFFFF) {
        dev_info(&client->dev, "At module loading, some buttons are pressed on chip 0: %04X\n", (gpio_data & 0xFFFF));
      }
      if (PRESSED(gpio_data, buttons_bits[PLAYER1][ALLINONE_BTN_COIN])) {
        dev_info(&client->dev, "reversing logic for P1 COIN button on jamma!\n");
        buttonsReleasedValues[buttons_bits[PLAYER1][ALLINONE_BTN_COIN]] = 0;
      }
    }

    // Ouput / inputs GPIOs
    u_int32_t disable_amp = 0;
    of_property_read_u32(client->dev.of_node, "disable_amp", &disable_amp);

    allinone_config.enableDAC = devm_gpiod_get_index(&client->dev, "aio_controls", 0, GPIOD_OUT_LOW);
    if (!allinone_config.enableDAC) printk(KERN_INFO "allinone: enableDAC failed\n");
    else
      gpiod_set_value(allinone_config.enableDAC, 1);
    printk(KERN_INFO "allinone: enableDAC %d\n", gpiod_get_value(allinone_config.enableDAC));

    allinone_config.disableAMP = devm_gpiod_get_index(&client->dev, "aio_controls", 1, GPIOD_OUT_LOW);
    if (!allinone_config.disableAMP) printk(KERN_INFO "allinone: disableAMP failed\n");
    else
      gpiod_set_value(allinone_config.disableAMP, disable_amp);
    printk(KERN_INFO "allinone: disableAMP %d\n", gpiod_get_value(allinone_config.disableAMP));

    allinone_config.util = devm_gpiod_get_index(&client->dev, "aio_controls", 2, GPIOD_IN);
    if (!allinone_config.util) printk(KERN_INFO "allinone: util failed\n");
    printk(KERN_INFO "allinone: util");

    u_int32_t amp_boost_1 = 0;
    of_property_read_u32(client->dev.of_node, "amp_boost_1", &amp_boost_1);
    allinone_config.gain0 = devm_gpiod_get_index(&client->dev, "aio_controls", 3, GPIOD_OUT_LOW);
    if (!allinone_config.gain0) printk(KERN_INFO "allinone: gain0 failed\n");
    else
      gpiod_set_value(allinone_config.gain0, amp_boost_1);

    u_int32_t amp_boost_2 = 0;
    of_property_read_u32(client->dev.of_node, "amp_boost_2", &amp_boost_2);
    allinone_config.gain1 = devm_gpiod_get_index(&client->dev, "aio_controls", 4, GPIOD_OUT_LOW);
    if (!allinone_config.gain1) printk(KERN_INFO "allinone: gain1 failed\n");
    else
      gpiod_set_value(allinone_config.gain1, amp_boost_2);

    // Specific configuration from dt
    of_property_read_u32(client->dev.of_node, "hk_on_start", &allinone_config.hk_on_start);
    of_property_read_u32(client->dev.of_node, "sound_on_start", &allinone_config.sound_on_start);
    of_property_read_u32(client->dev.of_node, "credit_on_start_btn1", &allinone_config.credit_on_start_btn1);
    of_property_read_u32(client->dev.of_node, "exit_on_start", &allinone_config.exit_on_start);
    of_property_read_u32(client->dev.of_node, "autofire", &allinone_config.autofire);
    of_property_read_u32(client->dev.of_node, "p1_credits_also_p2", &allinone_config.p1_credits_also_p2);

    of_property_read_u32(client->dev.of_node, "buttons", &allinone_config.p1_buttons);

/*    struct device_node *node = of_find_node_by_name(NULL, "aio_led_controller_p1");

    if (node) {
      of_property_read_u32(client->dev.of_node, "buttons", &allinone_config.p1_buttons);
    }*/

    dev_info(&client->dev,
             "All In One configuration: \n- disable_amp: %d\n- amp_boost_1: %d\n- amp_boost_2: %d\n- hk_on_start: %d \n- sound_on_start: %d \n- credit_on_start_btn1: %d \n- exit_on_start: %d \n"
             "- autofire: %d \n- p1_credits_also_p2: %d ",
             disable_amp, amp_boost_1, amp_boost_2, allinone_config.hk_on_start, allinone_config.sound_on_start,
             allinone_config.credit_on_start_btn1,
             allinone_config.exit_on_start, allinone_config.autofire, allinone_config.p1_credits_also_p2);
    // TODO strategie de config:
    // Si pas 1 seul coin -> double coin ?
    // Si pas de hotkey -> start = hotkey
    // ou config manuelle ?

  } else if (client->addr == 0x21) {
    dev_info(&client->dev, "chip 2\n");

    allinone_config.gpio_chip_1 = &chip->gpio_chip;
    if (process_inputs(&chip->gpio_chip) == 0) {
      if (((gpio_data >> 16) & 0xFFFF) != 0xFFFF) {
        dev_info(&client->dev, "At module loading, some buttons are pressed on chip 1: %04X\n", (gpio_data >> 16) & 0xFFFF);
      }
    }
    of_property_read_u32(client->dev.of_node, "buttons", &allinone_config.p2_buttons);
  }

  if (allinone_config.gpio_chip_0 && allinone_config.gpio_chip_1) {

    printk(KERN_INFO
           "allinone: all chips configured\n");
    int idx = 0;
    allinone_config.debounce_thread = kthread_create(process_debounce_and_turbo, &idx, "kthread_allinone_debounce");
    printk(KERN_INFO
           "allinone: setting debounce thread\n");
    if (allinone_config.debounce_thread != NULL) {
      wake_up_process(allinone_config.debounce_thread);
      printk(KERN_INFO
             "allinone: kthread_allinone_debounce is running\n");
    } else {
      printk(KERN_ERR
             "allinone: kthread_allinone_debounce could not be created\n");
      return -1;
    }

    allinone_config.config_thread = kthread_create(watch_configuration, &idx, "kthread_allinone_cfg");
    printk(KERN_INFO
           "allinone: setting configuration thread\n");
    if (allinone_config.config_thread != NULL) {
      wake_up_process(allinone_config.config_thread);
      printk(KERN_INFO
             "allinone: kthread_allinone_cfg is running\n");
    } else {
      printk(KERN_ERR
             "allinone: kthread kthread_allinone_cfg could not be created\n");
      return -1;
    }
  }

  return 0;

err_exit:
  regulator_disable(chip->regulator);
  return ret;
}

static void pca953x_remove(struct i2c_client *client) {
  struct pca953x_chip *chip = i2c_get_clientdata(client);
  regulator_disable(chip->regulator);
}

static const struct of_device_id pca953x_dt_ids[] = {
    {.compatible = "raspberrypi,allinone", .data = (void *) (16 | PCA953X_TYPE | PCA_INT)},
    {}};

MODULE_DEVICE_TABLE(of, pca953x_dt_ids);
static SIMPLE_DEV_PM_OPS(pca953x_pm_ops, pca953x_suspend, pca953x_resume);

static struct i2c_driver pca953x_driver = {
    .driver = {
        .name = "allinone",
        .pm = &pca953x_pm_ops,
        .of_match_table = pca953x_dt_ids,
        .acpi_match_table = pca953x_acpi_ids,
    },
    .probe = pca953x_probe,
    .remove = pca953x_remove,
    .id_table = pca953x_id,
};

static int power_off(struct sys_off_data *data);

static int __init pca953x_init(void) {
  int idx = 0;
  allinone_config.gpio_chip_0 = NULL;
  allinone_config.gpio_chip_1 = NULL;
  allinone_config.hk_on_start = true;
  allinone_config.sound_on_start = true;
  allinone_config.credit_on_start_btn1 = true;
  allinone_config.exit_on_start = true;
  allinone_config.autofire = false;
  allinone_config.autofire_time = 50000000;// 50ms
  allinone_config.amp_disable = 0;
  allinone_config.amp_boost = 0;
  allinone_config.player_count = 2;
  allinone_config.dual_joy = false;
  allinone_config.debounce_time_ms = 10;

  // New for all in one
  allinone_config.p1_credits_also_p2 = false;
  allinone_config.p1_buttons = 6;
  allinone_config.p2_buttons = 6;

  mutex_init(&allinone_config.process_mutex);
  printk(KERN_INFO "allinone: registering controllers\n");
  mutex_lock(&allinone_config.process_mutex);
  register_controllers();
  mutex_unlock(&allinone_config.process_mutex);

  register_sys_off_handler(SYS_OFF_MODE_POWER_OFF, SYS_OFF_PRIO_DEFAULT, power_off, NULL);

  return i2c_add_driver(&pca953x_driver);
}

/* register after i2c/ postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(pca953x_init);

static void __exit pca953x_exit(void) {
  printk(KERN_INFO
         "allinone: exiting\n");

  if (allinone_config.config_thread) {
    if (kthread_stop(allinone_config.config_thread)) {
      printk("allinone: can't stop config thread");
    }
  }
  if (allinone_config.debounce_thread) {
    if (kthread_stop(allinone_config.debounce_thread)) {
      printk("allinone: can't stop debounce thread");
    }
  }
  i2c_del_driver(&pca953x_driver);
  mutex_lock(&allinone_config.process_mutex);
  unregister_controllers();
  mutex_unlock(&allinone_config.process_mutex);
}

static int power_off(struct sys_off_data *data) {
  pca953x_exit();
  return 0;
}

module_exit(pca953x_exit);

MODULE_AUTHOR("eric miao <eric.miao@marvell.com>, digitalLumberjack <digitalLumberjack@recalbox.com>");
MODULE_DESCRIPTION("allinone driver");
MODULE_LICENSE("GPL");