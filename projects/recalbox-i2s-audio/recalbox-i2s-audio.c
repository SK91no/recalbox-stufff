// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the PCM5102A codec
 * Modified for recalbox i2s dac
 *
 * Author:	Florian Meier <florian.meier@koalo.de>
 *		Copyright 2013
 * Author:	digitalLumberjack <digitalLumberjack@gmail.com>
 *		Copyright 2023
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>

static struct snd_soc_dai_driver recalbox_i2s_dai = {
  .name = "pcm5102a-hifi",
  .playback = {
    .channels_min = 8,
    .channels_max = 8,
    .rates = SNDRV_PCM_RATE_8000_384000,
    .formats = SNDRV_PCM_FMTBIT_S16_LE |
               SNDRV_PCM_FMTBIT_S24_LE |
               SNDRV_PCM_FMTBIT_S32_LE
  },
};

static struct snd_soc_component_driver soc_component_dev_recalbox_i2s = {
  .idle_bias_on		= 1,
  .use_pmdown_time	= 1,
  .endianness		= 1,
};

static int recalbox_i2s_probe(struct platform_device *pdev)
{
  return devm_snd_soc_register_component(&pdev->dev, &soc_component_dev_recalbox_i2s,
                                         &recalbox_i2s_dai, 1);
}

static const struct of_device_id recalbox_i2s_of_match[] = {
  { .compatible = "recalbox_i2s", },
  { }
};
MODULE_DEVICE_TABLE(of, recalbox_i2s_of_match);

static struct platform_driver recalbox_i2s_driver = {
  .probe		= recalbox_i2s_probe,
  .driver		= {
    .name	= "recalbox_i2s",
    .of_match_table = recalbox_i2s_of_match,
  },
};

module_platform_driver(recalbox_i2s_driver);

MODULE_DESCRIPTION("Recalbox I2S Audio");
MODULE_AUTHOR("Florian Meier <florian.meier@koalo.de>, digitalLumberjack <digitalLumberjack@gmail.com>");
MODULE_LICENSE("GPL v2");
