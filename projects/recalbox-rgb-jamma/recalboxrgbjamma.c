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
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_data/pca953x.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/reboot.h>

#include <linux/unaligned.h>

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

#define DEBUG 0
#define TRACE 0

static const struct i2c_device_id pca953x_id[] = {
    {"recalboxrgbjamma", 16 | PCA953X_TYPE | PCA_INT},
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
  struct gpio_chip *gpio_chip_2;
  struct gpio_chip *expander;
  struct task_struct *config_thread;
  struct task_struct *debounce_thread;
  struct mutex process_mutex;
  bool hk_on_start;
  bool credit_on_start_btn1;
  bool exit_on_start;
  bool sound_on_start;
  bool autofire;
  uint autofire_time;
  bool i2s;
  bool videofilter;
  uint buttons_on_jamma;
  uint amp_boost;
  bool amp_disable;
  uint player_count;
  bool dual_joy;
  uint debounce_time_ms;
  uint version;
  bool coin_counter;
  bool unplugged;
} jamma_config;



/* CONTROLS
 *
 * Chip 0 : address = 0x25
 *                                    __________
 *   P4K-LOCK            IO0-0  <--- |0        8| ---> IO1-0  P3K-B4
 *   P4K-COIN            IO0-1  <--- |1        9| ---> IO1-1  P4K-B4
 *   P4K-START           IO0-2  <--- |2       10| ---> IO1-2  P4K-B3
 *   P4K-UP              IO0-3  <--- |3       11| ---> IO1-3  P4K-B2
 *   P4K-DOWN            IO0-4  <--- |4       12| ---> IO1-4  P2-B3 (P2-Y)
 *   P4K-LEFT            IO0-5  <--- |5       13| ---> IO1-5  P1-B3 (P1-Y)
 *   P4K-RIGHT           IO0-6  <--- |6       14| ---> IO1-6  P2-B2 (P2-A)
 *   P4K-B1              IO0-7  <--- |7       15| ---> IO1-7  P1-B2 (P1-A)
 *                                    ‾‾‾‾‾‾‾‾‾‾
 *
 * Chip 1 : address = 0x24
 *                                    __________
 *   P2-B4 (P2-X)        IO0-0  <--- |16      24| ---> IO1-0  P1K-B4/P3-RIGHT
 *   P1-B4 (P1-X)        IO0-1  <--- |17      25| ---> IO1-1  P1K-B5/P3-LEFT
 *   P1-B5 (P1-L)        IO0-2  <--- |18      26| ---> IO1-2  P1K-B6/P3-DOWN
 *   P2-B5 (P2-L)        IO0-3  <--- |19      27| ---> IO1-3  P2K-B4/P3-B1
 *   P3K-B3              IO0-4  <--- |20      28| ---> IO1-4  P2K-B5/P3-B2
 *   P3K-UP              IO0-5  <--- |21      29| ---> IO1-5  P2K-B6/P3-COIN
 *   P3K-START           IO0-6  <--- |22      30| ---> IO1-6  P1-B6  (P1-R)
 *   P3K-LOCK            IO0-7  <--- |23      31| ---> IO1-7  P2-B6  (P2-R)
 *                                    ‾‾‾‾‾‾‾‾‾‾
 *
 * Chip 2 : address = 0x27
 *                                    __________
 *   P2-B1               IO0-0  <--- |32      40| ---> IO1-0  P1-UP
 *   P1-B1               IO0-1  <--- |33      41| ---> IO1-1  P2-UP
 *   P2-RIGHT            IO0-2  <--- |34      42| ---> IO1-2  P2-START
 *   P1-RIGHT            IO0-3  <--- |35      43| ---> IO1-3  P1-START
 *   P2-LEFT             IO0-4  <--- |36      44| ---> IO1-4  P2-COIN
 *   P2-DOWN             IO0-5  <--- |37      45| ---> IO1-5  P1-COIN
 *   P1-DOWN             IO0-6  <--- |38      46| ---> IO1-6  TEST
 *   P1-LEFT             IO0-7  <--- |39      47| ---> IO1-7  SERVICE
 *                                    ‾‾‾‾‾‾‾‾‾‾
 *
 * Chip 3 : address = 0x26
 *                               __________
 *                         <--- |0        8| ---> IO1-0     AMP_DISABLE
 *   I2S_FILTER     IO0-1  <--- |1        9| ---> IO1-1     GAIN1
 *   PI5_I2S        IO0-2  <--- |2       10| ---> IO1-2     GAIN0
 *                  IO0-3  <--- |3       11| ---> IO1-3     EXTRA4
 *                  IO0-4  <--- |4       12| ---> IO1-4     EXTRA3
 *                  IO0-5  <--- |5       13| ---> IO1-5     EXTRA2
 *                  IO0-6  <--- |6       14| ---> IO1-6     EXTRA1
 *   VIDEO_BYPASS   IO0-7  <--- |7       15| ---> IO1-7     FAN
 *                               ‾‾‾‾‾‾‾‾‾‾
 *
 *
 * RGBJAMMA 2 - PROTO 1
 * Chip 3 : address = 0x26
 *                               __________
 *   I2S_FILTER     IO0-1  <--- |0        8| ---> IO1-0     GPIO-CC-P1
 *   PI5_I2S        IO0-1  <--- |1        9| ---> IO1-1     GPIO-CC-P2
 *   I2CINT1        IO0-2  <--- |2       10| ---> IO1-2     VIDEO_BYPASS
 *   VERSION0       IO0-3  <--- |3       11| ---> IO1-3     I2CINT3
 *   VERSION1       IO0-4  <--- |4       12| ---> IO1-4     AMP_DISABLE
 *   VERSION2       IO0-5  <--- |5       13| ---> IO1-5     GAIN1
 *   VERSION3       IO0-6  <--- |6       14| ---> IO1-6     GAIN0
 *   I2CINT2        IO0-7  <--- |7       15| ---> IO1-7     FAN
 *                               ‾‾‾‾‾‾‾‾‾‾
 *
 *
 * RGBJAMMA 2 - REV A
 * Chip 3 : address = 0x26
 *                               __________
 *   PI5_I2S        IO0-1  <--- |0        8| ---> IO1-0     GPIO-CC-P1
 *   I2CINT1        IO0-1  <--- |1        9| ---> IO1-1     GPIO-CC-P2
 *   I2CINT2        IO0-2  <--- |2       10| ---> IO1-2     VIDEO_BYPASS
 *   I2CINT3        IO0-3  <--- |3       11| ---> IO1-3     I2S_FILTER
 *   VERSION0       IO0-4  <--- |4       12| ---> IO1-4     AMP_DISABLE
 *   VERSION1       IO0-5  <--- |5       13| ---> IO1-5     GAIN1
 *   VERSION2       IO0-6  <--- |6       14| ---> IO1-6     GAIN0
 *   VERSION3       IO0-7  <--- |7       15| ---> IO1-7     FAN
 *                               ‾‾‾‾‾‾‾‾‾‾
 */

/* EXPANDER */
#define EXP_I2S_FILTER 0
#define EXP_PI5_I2S 1
#define EXP_CC_P1 2
#define EXP_CC_P2 3
#define EXP_VIDEO_BYPASS 4
#define EXP_AMP_DISABLE 5
#define EXP_GAIN1 6
#define EXP_GAIN0 7
#define EXP_FAN 8
/* Expander INTERUPS */
#define EXP_I2CINT1_GPIO 9
#define EXP_I2CINT2_GPIO 10
#define EXP_I2CINT3_GPIO 11

#define RRGBJ1 0
#define RRGBJ2PROTO1 1
#define RRGBJ2REVA 2
static bool is_rrgbj2(void) {
  return jamma_config.version >= RRGBJ2PROTO1;
}

static uint8_t expander[3][12] = {
    // RGB JAMMA 1
    // EXP_I2S_FILTER, EXP_PI5_I2S, EXP_CC_P1, EXP_CC_P2, EXP_VIDEO_BYPASS, EXP_AMP_DISABLE, EXP_GAIN1, EXP_GAIN0, EXP_FAN, EXP_I2CINT1_GPIO, EXP_I2CINT2_GPIO, EXP_I2CINT3_GPIO
    {  1             , 2          , -1       , -1       , 7               , 8              , 9        , 10       , 14     , -1              , -1              , -1                },
    // RGB JAMMA 2 PROTO 1
    // EXP_I2S_FILTER, EXP_PI5_I2S, EXP_CC_P1, EXP_CC_P2, EXP_VIDEO_BYPASS, EXP_AMP_DISABLE, EXP_GAIN1, EXP_GAIN0, EXP_FAN, EXP_I2CINT1_GPIO, EXP_I2CINT2_GPIO, EXP_I2CINT3_GPIO
    {  0             , 1          , 8        , 9        , 10              , 12             , 13       , 14       , 15     , 2               , 7               , 11                },
    // RGB JAMMA 2 REVA
    // EXP_I2S_FILTER, EXP_PI5_I2S, EXP_CC_P1, EXP_CC_P2, EXP_VIDEO_BYPASS, EXP_AMP_DISABLE, EXP_GAIN1, EXP_GAIN0, EXP_FAN, EXP_I2CINT1_GPIO, EXP_I2CINT2_GPIO, EXP_I2CINT3_GPIO
    {  11            ,  0         , 8        , 9        , 10              , 12             , 13       , 14       , 15     , 1               , 2               , 3                 },
};


static irqreturn_t pca953x_irq_handler(int irq, void *devid) {

  struct pca953x_chip *chip = (struct pca953x_chip *) devid;
  if (&chip->gpio_chip == jamma_config.gpio_chip_0) {
    DEBUG &&printk(KERN_INFO
                   "recalboxrgbjamma: IRQ triggered on chip 0\n");
    process_inputs(jamma_config.gpio_chip_0);
  } else if (&chip->gpio_chip == jamma_config.gpio_chip_1) {
    DEBUG &&printk(KERN_INFO
                   "recalboxrgbjamma: IRQ triggered on chip 1\n");
    process_inputs(jamma_config.gpio_chip_1);
  } else if (&chip->gpio_chip == jamma_config.gpio_chip_2) {
    DEBUG &&printk(KERN_INFO
                   "recalboxrgbjamma: IRQ triggered on chip 2\n");
    process_inputs(jamma_config.gpio_chip_2);
  } else if (&chip->gpio_chip == jamma_config.expander) {
    DEBUG &&printk(KERN_INFO
                   "recalboxrgbjamma: IRQ triggered on expander (rrgbj2)\n");
    unsigned long _mask = 0xFFFF;
    unsigned long _gpio_data = 0x0;
    bool read = false;
    if (pca953x_gpio_get_multiple(jamma_config.expander, &_mask, &_gpio_data)) {
      printk(KERN_INFO "recalboxrgbjamma: unable to read registers on expander for interupts\n");
    }
    DEBUG && printk(KERN_INFO "recalboxrgbjamma: expander data = %04X\n", _gpio_data);
    if ((_gpio_data & (1<<expander[jamma_config.version][EXP_I2CINT1_GPIO])) == 0){
      DEBUG && printk(KERN_INFO "recalboxrgbjamma: chip 0 irq\n");
      process_inputs(jamma_config.gpio_chip_0);
      read = true;
    }
    if ((_gpio_data & (1<<expander[jamma_config.version][EXP_I2CINT2_GPIO])) == 0){
      DEBUG && printk(KERN_INFO "recalboxrgbjamma: chip 1 irq\n");
      process_inputs(jamma_config.gpio_chip_1);
      read = true;

    }
    if ((_gpio_data & (1<<expander[jamma_config.version][EXP_I2CINT3_GPIO])) == 0){
      DEBUG && printk(KERN_INFO "recalboxrgbjamma: chip 2 irq\n");
      process_inputs(jamma_config.gpio_chip_2);
      read = true;
    }
    if (!read) {
      DEBUG && printk(KERN_INFO "recalboxrgbjamma: irq: reading all inputs\n");
      process_inputs(NULL);
    }
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
  irq_chip->flags = IRQCHIP_IMMUTABLE;

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

  printk(KERN_INFO
       "recalboxrgbjamma: IRQ SET\n");
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

#define MAX_PLAYERS 4

struct input_dev *player_devs[MAX_PLAYERS];

// Standard buttons mapping
#define BTN_PER_PLAYER 13
#define BTN_PER_PLAYER_ON_JAMMA 10
#define TOTAL_GPIO_ON_PCA 48

// Keep in mind the vanilla mapping for Recalbox RGB Jamma is (SNES notation):
// B A Y
// X L R
// We use this mapping to get B and A on first line, whatever the number of buttons
// Keep also in mind that the A is SOUTH and B is EAST (they are switched) on Linux notation, and that NORTH and WEST are also reversed in auto mapping...
//
static const unsigned int buttons_codes[BTN_PER_PLAYER] = {
    BTN_A,         //  JAMMA_BTN_1
    BTN_B,         //  JAMMA_BTN_2
    BTN_X,         //  JAMMA_BTN_3
    BTN_Y,         //  JAMMA_BTN_4
    BTN_TL,        //  JAMMA_BTN_5
    BTN_TR,        //  JAMMA_BTN_6
    BTN_START,     //  JAMMA_BTN_START
    BTN_SELECT,    //  JAMMA_BTN_COIN
    BTN_THUMBL,    //  JAMMA_BTN_SERVICE
    BTN_THUMBR,    //  JAMMA_BTN_TEST
    BTN_MODE,      //  BTN_HOTKEY
    KEY_VOLUMEUP,  //  VOLUME_UP
    KEY_VOLUMEDOWN,//  VOLUME_DOWN
};

#define JAMMA_BTN_1 0
#define JAMMA_BTN_2 1
#define JAMMA_BTN_3 2
#define JAMMA_BTN_4 3
#define JAMMA_BTN_5 4
#define JAMMA_BTN_6 5
#define JAMMA_BTN_START 6
#define JAMMA_BTN_COIN 7
#define JAMMA_BTN_SERVICE 8
#define JAMMA_BTN_TEST 9
#define BTN_HOTKEY 10
#define VOLUME_UP 11
#define VOLUME_DOWN 12

// Games buttons are from 0 to 5 in buttons codes
#define LAST_GAME_BUTTON 5


#define PLAYER1 0
#define PLAYER2 1
#define PLAYER3 2
#define PLAYER4 3
#define JAMMA_BTNS 0
#define KICK_BTNS 1

static int buttons_bits[MAX_PLAYERS][2][BTN_PER_PLAYER_ON_JAMMA] = {
    // Player 1
    {
        // The value is the bit of the data containing the
        // information for the button at the same index in buttons_codes array
        // Start is a special case
        {33, 15, 13, 17, 18, 30, -1, 45, 46, 47},
        // Kick harness buttons mapping
        // Only use 3 btns here
        {-1, -1, -1, 24, 25, 26, -1, -1, -1, -1},
    },
    // Player 2
    {
        {32, 14, 12, 16, 19, 31, 42, 44, -1, -1},
        {-1, -1, -1, 27, 28, 29, -1, -1, -1, -1},
    },
    // Player 3
    {
        //1   2   3   4   5   6  ST  CON  TL  TR
        {27, 28, 20, 8, 23, -1, 22, 29, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    },
    // Player 4
    {
        //1   2   3   4   5   6  ST  CON  TL  TR
        {7, 11, 10, 9, 0, -1, 2, 1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    }};

#define P1_START 43
#define P1_SERVICE buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_SERVICE]
#define P1_TEST buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_TEST]

#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

static const int direction_bits[MAX_PLAYERS][4] = {
    // Player 1
    {40, 38, 39, 35},
    {41, 37, 36, 34},
    {21, 26, 25, 24},
    {3, 4, 5, 6},
};


// Logic is not the same for each buttons (coins and service/test are reversed)
// TODO: add service and test for next version
static unsigned short buttonsReleasedValues[TOTAL_GPIO_ON_PCA] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1};


#define PRESSED(data, btn) ((btn) != -1 ? (((data >> (btn)) & 1) ^ (buttonsReleasedValues[btn])) : 0)
#define ANYPRESSED(data, player, btn) (PRESSED(data, buttons_bits[player][JAMMA_BTNS][btn]) || PRESSED(data, buttons_bits[player][KICK_BTNS][btn]))

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
#define HOTKEY_PATTERNS_ENABLED(jamma_config) (jamma_config.credit_on_start_btn1 || jamma_config.hk_on_start || jamma_config.exit_on_start || jamma_config.sound_on_start)

static long long press_time[MAX_PLAYERS][BTN_PER_PLAYER_ON_JAMMA] = {0};
static bool turbo_enabled[MAX_PLAYERS][BTN_PER_PLAYER_ON_JAMMA] = {0};
static long long turbo_time[MAX_PLAYERS][BTN_PER_PLAYER_ON_JAMMA] = {0};
static bool turbo_state[MAX_PLAYERS][BTN_PER_PLAYER_ON_JAMMA] = {0};


void manage_special_inputs(unsigned long long *data_chips, long long int *time_ns) {
  // Special case for START pattern:
  // Simple press + release of START -> START event
  // Simple press of START + BTN1 -> send SELECT (COIN) on release
  // Simple press of START + an other button -> send HOTKEY + BUTTON
  // Simple press of START + an other button 3sec -> auto fire on button
  // Long press (3s) of START -> send HOTKEY + START
  if (PRESSED(*data_chips, P1_START)) {
    DEBUG &&printk(KERN_INFO "recalboxrgbjamma: START is pressed\n");
    if (last_start_press == 0) {
      // First press
      DEBUG &&printk(KERN_INFO "recalboxrgbjamma: saving START first press\n");
      last_start_press = *time_ns;
      start_state = *data_chips;
      should_release_start = 1;
      should_release_hk = 0;
      can_exit = jamma_config.exit_on_start;
    } else if (start_state != *data_chips) {
      // Start is pressed and the state has changed since last loop
      // Only player 1
      int player = PLAYER1;
      for (int button = 0; button < BTN_PER_PLAYER_ON_JAMMA; button++) {
        if (press_time[player][button] == 0 && ANYPRESSED(*data_chips, player, button)) {
          // Saving press time
          press_time[player][button] = *time_ns;
        } else if (press_time[player][button] > 0 && !ANYPRESSED(*data_chips, player, button)) {
          // button released while start pressed, we must act
          // Checking if simple press (time < 3sec)
          if (*time_ns - press_time[player][button] < 3000000000LL) {
            // Credit ?
            if (jamma_config.credit_on_start_btn1) {
              if (button == JAMMA_BTN_1) {
                printk(KERN_INFO "recalboxrgbjamma: credit (SELECT) triggered (START+BTN1) for player=%d\n", player);
                PRESS_AND_RELEASE(player, BTN_SELECT);
                can_exit = 0;
                should_release_start = 0;
              }
            }
            // Hotkey + BTN ?
            // Check if we should use START + BTN pattern (Hotkey)
            // Only if not disabled in config, and credit not already used, and this is the first hk event
            if (jamma_config.hk_on_start && should_release_start && !should_release_hk) {
              if (player == PLAYER1) {
                // As another button has been pressed, we press hotkey before
                printk(KERN_INFO "recalboxrgbjamma: sending HK + BTN%d\n", button);
                PRESS_AND_SYNC(PLAYER1, BTN_MODE);
                can_exit = 0;
                should_release_hk = 1;
              }
            }
          } else {
            // AUTO FIRE because press time > 3 sec
            if (jamma_config.autofire && should_release_start && !should_release_hk) {
              printk(KERN_INFO "recalboxrgbjamma: setting autofire for player %d on + BTN%d to %d\n", player, button, !turbo_enabled[player][button]);
              turbo_enabled[player][button] = !turbo_enabled[player][button];
              can_exit = 0;
            }
          }
          press_time[player][button] = 0;
        }
      }
      // Directions for volume
      if (jamma_config.sound_on_start) {
        for (int direction = DIR_UP; direction <= DIR_DOWN; direction++) {
          if (PRESSED(*data_chips, direction_bits[player][direction])) {
            // Volume (UP/DOWN)
            if (direction == DIR_UP) {
              printk(KERN_INFO "recalboxrgbjamma: VOLUME UP\n");
              PRESS_AND_RELEASE(player, buttons_codes[VOLUME_UP]);
              should_release_start = 0;
            } else if (direction == DIR_DOWN) {
              printk(KERN_INFO "recalboxrgbjamma: VOLUME DOWN\n");
              PRESS_AND_RELEASE(player, buttons_codes[VOLUME_DOWN]);
              should_release_start = 0;
            }
          }
        }
      } else if (jamma_config.hk_on_start && !should_release_hk) {
        // Directions up down if not volume on start enabled but hk on start is enabled
        for (int direction = DIR_UP; direction <= DIR_DOWN; direction++) {
          if (PRESSED(*data_chips, direction_bits[player][direction])) {
            printk(KERN_INFO
            "recalboxrgbjamma: sending HK + DIRECTION %d\n", direction);
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
            printk(KERN_INFO "recalboxrgbjamma: sending HK + DIRECTION %d\n", direction);
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
                       "recalboxrgbjamma: start released\n");
        if (should_release_hk) {
          DEBUG &&printk(KERN_INFO
                         "recalboxrgbjamma: release hotkey\n");
          should_release_hk = 0;
          RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
        } else if (can_exit && EXIT_DELAY(*time_ns, last_start_press)) {
          // Longest press, so we send both HK + START
          DEBUG &&printk(KERN_INFO
                         "recalboxrgbjamma: long press : sending HK + START\n");
          PRESS_AND_SYNC(PLAYER1, BTN_MODE);
          PRESS_AND_SYNC(PLAYER1, BTN_START);
          RELEASE_AND_SYNC(PLAYER1, BTN_START);
          RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
        } else if (HOTKEY_DELAY(*time_ns, last_start_press)) {
          // Long press, so we send HK
          DEBUG &&printk(KERN_INFO
                         "recalboxrgbjamma: middle press : sending HK\n");
          PRESS_AND_RELEASE(PLAYER1, BTN_MODE);
        } else {
          // Simple start
          DEBUG &&printk(KERN_INFO
                         "recalboxrgbjamma: quick press: sending START\n");
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
  for (int player = 1; player < jamma_config.player_count; player++) {
    if (PRESSED(*data_chips, buttons_bits[player][JAMMA_BTNS][JAMMA_BTN_START])) {
      for (int button = 0; button < LAST_GAME_BUTTON; button++) {
        if (press_time[player][button] == 0 && ANYPRESSED(*data_chips, player, button)) {
          // Saving press time
          press_time[player][button] = *time_ns;
        } else if (press_time[player][button] > 0 && !ANYPRESSED(*data_chips, player, button)) {
          // button released while start pressed, we must act
          // Checking if simple press (time < 3sec)
          if (jamma_config.credit_on_start_btn1 && *time_ns - press_time[player][button] < 3000000000LL) {
            // Credit ?
            if (button == JAMMA_BTN_1) {
              printk(KERN_INFO "recalboxrgbjamma: credit (SELECT) triggered (START+BTN1) for player=%d\n", player);
              PRESS_AND_RELEASE(player, BTN_SELECT);
            }
          } else if (jamma_config.autofire && *time_ns - press_time[player][button] >= 3000000000LL){
            // AUTO FIRE because press time > 3 sec
            printk(KERN_INFO "recalboxrgbjamma: setting autofire for player %d on + BTN%d to %d\n", player, button, !turbo_enabled[player][button]);
            turbo_enabled[player][button] = !turbo_enabled[player][button];
          }
          press_time[player][button] = 0;
        }
      }
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
  if (HOTKEY_PATTERNS_ENABLED(jamma_config)) {
    buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_START] = -1;
    manage_special_inputs(data_chips, time_ns);
  } else {
    buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_START] = P1_START;
  }*/
  // Always seek special inputs as HK + LEFT RIGHT is always enabled (for clones)
  buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_START] = -1;
  manage_special_inputs(data_chips, time_ns);

  // Exit with TEST + SERVICE
  if (PRESSED(*data_chips, P1_SERVICE) && PRESSED(*data_chips, P1_TEST)) {
    printk(KERN_INFO "recalboxrgbjamma: Exiting with SERVICE + TEST\n");
    PRESS_AND_SYNC(PLAYER1, BTN_MODE);
    PRESS_AND_SYNC(PLAYER1, BTN_START);
    RELEASE_AND_SYNC(PLAYER1, BTN_START);
    RELEASE_AND_SYNC(PLAYER1, BTN_MODE);
    // Adding player 2 as it may be set as player 1
    PRESS_AND_SYNC(PLAYER2, BTN_MODE);
    PRESS_AND_SYNC(PLAYER2, BTN_START);
    RELEASE_AND_SYNC(PLAYER2, BTN_START);
    RELEASE_AND_SYNC(PLAYER2, BTN_MODE);
  }


  for (player = 0; player < jamma_config.player_count; player++) {
    // Only process P1 if start + credit is not running
    if (player > PLAYER1 || should_release_start) {
      if(jamma_config.dual_joy){
        if(player == PLAYER2) {
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

      for (buttonIndex = 0; buttonIndex < BTN_PER_PLAYER_ON_JAMMA; buttonIndex++) {
        // If we are on 4 player mode, all kick harness buttons are used for player 3 and 4, so player 1 and 2 have only jamma buttons.
        if (jamma_config.player_count == 4) {
          // Don't read player 1 and 2 buttons on jamma if they are not wired
          if (player < 2 && (buttonIndex <= LAST_GAME_BUTTON && buttonIndex >= jamma_config.buttons_on_jamma)) {
            continue;
          } else {
            // Read only on jamma for players 1 and 2. Player 3 and 4 are considered on JAMMA for algo purpose
            buttonValue = PRESSED(*data_chips, buttons_bits[player][JAMMA_BTNS][buttonIndex]);
          }
        } else {
          // If we are on a game button that is not on jamma (because user config)
          // then we use kickharness
          if ((buttonIndex <= LAST_GAME_BUTTON && buttonIndex >= jamma_config.buttons_on_jamma)) {
            buttonValue = PRESSED(*data_chips, buttons_bits[player][KICK_BTNS][buttonIndex]);
            /* DEBUG && printk(KERN_INFO "recalboxrgbjamma: sending report P1 for key bit:%d, inputcode:%d, value : %d\n", player1_kick_btn_bits[j],
             buttons_codes[j], PRESSED(data_chips, player1_kick_btn_bits[j]));*/
          } else {
            // let's read on both inputs
            buttonValue = ANYPRESSED(*data_chips, player, buttonIndex);
          }
        }

        input_report_key(player_devs[player], buttons_codes[buttonIndex], buttonValue);
      }
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
  if (jamma_config.gpio_chip_0 != NULL) {
    if (pca953x_gpio_get_multiple(jamma_config.gpio_chip_0, &mask, &gpio_values0)) {
      printk(KERN_INFO
             "recalboxrgbjamma: unable to read gpio on chip 0, skipping\n");
      return -1;
    };
  }
  if (jamma_config.gpio_chip_1 != NULL) {
    if (pca953x_gpio_get_multiple(jamma_config.gpio_chip_1, &mask, &gpio_values1)) {
      printk(KERN_INFO
             "recalboxrgbjamma: unable to read gpio chip 1, skipping\n");
      return -1;
    }
  }
  if (jamma_config.gpio_chip_2 != NULL) {
    if (pca953x_gpio_get_multiple(jamma_config.gpio_chip_2, &mask, (unsigned long *)&gpio_values2)) {
      printk(KERN_INFO
             "recalboxrgbjamma: unable to read gpio chip 2, skipping\n");
      return -1;
    }
  }
  *gpio_data = (gpio_values2 << 32) | (gpio_values1 << 16) | gpio_values0;
  return 0;
}

static int read_gpios_one_chip(u_int64_t *gpio_data, struct gpio_chip *gpio_chip) {
  if (pca953x_gpio_get_multiple(gpio_chip, &mask, (unsigned long *)gpio_data)) {
    printk(KERN_INFO
           "recalboxrgbjamma: unable to read gpio on chip %s, skipping\n",
           gpio_chip == jamma_config.gpio_chip_0 ? "0" : (gpio_chip == jamma_config.gpio_chip_1 ? "1" : "2"));
    return -1;
  };
  return 0;
}

static bool meterdone1 = false;
static bool meterdone2 = false;
static void manage_coin_meters(u_int64_t *data_chips) {
  // Coin conter
  if(jamma_config.coin_counter && is_rrgbj2()){
    if(PRESSED(*data_chips, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_COIN]) && !meterdone1) {
      printk(KERN_INFO "recalboxrgbjamma: manage_coin_meters: triggering coin meter P1\n");
      pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P1], 1);
      usleep_range(100000, 150000);
      pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P1], 0);
      meterdone1 = true;
    } else {
      meterdone1 = false;
    }
    if(PRESSED(*data_chips, buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_COIN]) && !meterdone2) {
      printk(KERN_INFO "recalboxrgbjamma: manage_coin_meters: triggering coin meter P2\n");
      pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P2], 1);
      usleep_range(100000, 150000);
      pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P2], 0);
      meterdone2 = true;
    } else {
      meterdone2 = false;
    }
  }
}


static u_int64_t gpio_data = 0xFFFFFFFFFFFF;

// No mutex lock in read_inputs
static int read_inputs(struct gpio_chip *gpio_chip) {
  u_int64_t gpio_data_chip = 0;
  if (gpio_chip != NULL) {
    if (read_gpios_one_chip(&gpio_data_chip, gpio_chip)) {
      printk(KERN_ERR "recalboxrgbjamma: read_inputs: unable to read gpio chip, unlocking mutex\n");
      return -1;
    }
    DEBUG && printk(KERN_INFO "recalboxrgbjamma: read_inputs: read gpio values on chip : %llu\n", gpio_data_chip);
    if (gpio_chip == jamma_config.gpio_chip_0) {
      gpio_data = ((gpio_data & 0xFFFFFFFF0000) | gpio_data_chip);
    } else if (gpio_chip == jamma_config.gpio_chip_1) {
      gpio_data = ((gpio_data & 0xFFFF0000FFFF) | (gpio_data_chip << 16));
    } else if (gpio_chip == jamma_config.gpio_chip_2) {
      gpio_data = ((gpio_data & 0x0000FFFFFFFF) | (gpio_data_chip << 32));
    }
    DEBUG && printk(KERN_INFO "recalboxrgbjamma: read_inputs: gpio_data = %llu\n", gpio_data);
  } else {
    if (read_gpios(&gpio_data)) {
      printk(KERN_ERR "recalboxrgbjamma: read_inputs: unable to read gpio chips in batch\n");
      return -1;
    }
    DEBUG && printk(KERN_INFO "recalboxrgbjamma: read_inputs: gpio_data = %012llX\n", gpio_data);
  }
  return 0;
}

static int process_inputs_no_read(struct gpio_chip *gpio_chip) {
  u_int64_t debounced_data = 0;
  u_int64_t gpio_data_chip = 0;
  long long int time_start = ktime_to_ns(ktime_get_boottime());
  DEBUG &&printk(KERN_INFO "recalboxrgbjamma: process_inputs_no_read: locking mutex\n");
  mutex_lock(&jamma_config.process_mutex);
  for (int i = PLAYER1; i < jamma_config.player_count; i++) {
    if (player_devs[i] == NULL) {
      printk(KERN_INFO "recalboxrgbjamma: process_inputs_no_read: some players are NULL, unable to process inputs\n");
      mutex_unlock(&jamma_config.process_mutex);
      return 0;
    }
  }
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: debounce mutex\n");
  debounce(&gpio_data, &debounced_data, &time_start);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: input_report\n");
  input_report(&debounced_data, &time_start);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: manage_coin_meters\n");
  manage_coin_meters(&debounced_data);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: unlocking mutex\n");
  mutex_unlock(&jamma_config.process_mutex);
  return 0;
}

static int process_inputs(struct gpio_chip *gpio_chip) {
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: read_inputs\n");
  mutex_lock(&jamma_config.process_mutex);
  if (read_inputs(gpio_chip) != 0) {
    printk(KERN_INFO "recalboxrgbjamma: process_inputs: unable to read inputs\n");
    mutex_unlock(&jamma_config.process_mutex);
    return -1;
  }
  mutex_unlock(&jamma_config.process_mutex);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_inputs: process_inputs_no_read\n");
  return process_inputs_no_read(gpio_chip);
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
        if (time_ns - lastActionOn[button] > (1000000ul * jamma_config.debounce_time_ms)) {
          // This gpio must be debounced
          TRACE && printk(KERN_INFO "recalboxrgbjamma: process_debounce_and_turbo: debouncing thread: finish debouncing of %d\n", button);
          must_process_input = 1;
          lastActionOn[button] = 0;
        }
      }
    }
    if (jamma_config.autofire) {
      for (int player = 0; player < jamma_config.player_count; player++) {
        if((player == PLAYER1 && PRESSED(gpio_data, P1_START)) || PRESSED(gpio_data, buttons_bits[player][JAMMA_BTNS][JAMMA_BTN_START])){
          // Using turbo only if start is not pressed
          continue;
        }
        must_send_turbo = false;
        for (button = 0; button < BTN_PER_PLAYER_ON_JAMMA; button++) {
          if (turbo_enabled[player][button] && (PRESSED(gpio_data, buttons_bits[player][JAMMA_BTNS][button]) || PRESSED(gpio_data, buttons_bits[player][KICK_BTNS][button])) && time_ns - turbo_time[player][button] > jamma_config.autofire_time) {
            turbo_state[player][button] = !turbo_state[player][button];
            turbo_time[player][button] = time_ns;
            /*printk(KERN_INFO
                   "recalboxrgbjamma: turbo mode, setting state of button %d to %d\n",
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
      DEBUG && printk(KERN_INFO "recalboxrgbjamma: process_debounce_and_turbo: must_process_input -> reading inputs\n");
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

static const char *controllers_names[MAX_PLAYERS] = {"JammaControllerP1", "JammaControllerP2", "JammaControllerP3", "JammaControllerP4"};

static int register_controllers(void) {
  int err = 0, i = 0, player = 0;
  for (player = 0; player < jamma_config.player_count; player++) {
    if (!(player_devs[player] = input_allocate_device())) {
      pr_err("recalboxrgbjamma: not enough memory for input device\n");
      return -ENOMEM;
    }
    printk(KERN_INFO "recalboxrgbjamma: register player %d controller\n", player);

    player_devs[player]->name = controllers_names[player];
    player_devs[player]->phys = controllers_names[player];
    player_devs[player]->id.bustype = BUS_I2C;
    player_devs[player]->id.vendor = 0x7262;
    player_devs[player]->id.product = 0x0A01;
    player_devs[player]->id.version = 0x0100;

    player_devs[player]->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

    for (i = 0; i < 2; i++) {
      input_set_abs_params(player_devs[player], ABS_X + i, -1, 1, 0, 0);
      input_set_abs_params(player_devs[player], ABS_RX + i, -1, 1, 0, 0);
    }
    for (i = 0; i < BTN_PER_PLAYER; i++)
      __set_bit(buttons_codes[i], player_devs[player]->keybit);

    if ((err = input_register_device(player_devs[player]))) {
      pr_err("recalboxrgbjamma: Cannot register input device\n");
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
    //printk(KERN_INFO "recalboxrgbjamma: config file not found, skipping configuration loading\n");
    return 0;
  }

  read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
  if (read_size <= 0) {
    filp_close(fp, NULL);
    //printk(KERN_INFO "recalboxrgbjamma: empty config file found, skipping configuration loading\n");
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
            if (jamma_config.hk_on_start != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch hk_on_start to %d\n", optionvalue);
              jamma_config.hk_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.sound_on_start") == 0) {
            if (jamma_config.sound_on_start != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch sound_on_start to %d\n", optionvalue);
              jamma_config.sound_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.credit_on_start_btn1") == 0) {
            if (jamma_config.credit_on_start_btn1 != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch credit_on_start_btn1 to %d\n", optionvalue);
              jamma_config.credit_on_start_btn1 = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.exit_on_start") == 0) {
            if (jamma_config.exit_on_start != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch exit_on_start to %d\n", optionvalue);
              jamma_config.exit_on_start = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.buttons_on_jamma") == 0) {
            if (jamma_config.buttons_on_jamma != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch buttons_on_jamma to %d\n", optionvalue);
              jamma_config.buttons_on_jamma = optionvalue;
              if (jamma_config.buttons_on_jamma < 6) {
                printk(KERN_INFO "recalboxrgbjamma: setting BTN6 to output GND\n");
                pca953x_gpio_direction_output(jamma_config.gpio_chip_1, 14, 0);
                pca953x_gpio_direction_output(jamma_config.gpio_chip_1, 15, 0);
              } else {
                printk(KERN_INFO "recalboxrgbjamma: setting BTN6 to input\n");
                pca953x_gpio_direction_input(jamma_config.gpio_chip_1, 14);
                pca953x_gpio_direction_input(jamma_config.gpio_chip_1, 15);
              }
            }
          } else if (strcmp(optionname, "options.jamma.controls.autofire") == 0) {
            if (jamma_config.autofire != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch autofire to %d\n", optionvalue);
              jamma_config.autofire = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.autofire_ms") == 0) {
            if (jamma_config.autofire_time != optionvalue * 1000000) {
              printk(KERN_INFO "recalboxrgbjamma: switch autofire_ms to %d\n", optionvalue * 1000000);
              jamma_config.autofire_time = optionvalue * 1000000;
            }
          } else if (strcmp(optionname, "options.jamma.controls.4players") == 0) {
            if ((jamma_config.player_count == 2 && optionvalue == 1) || (jamma_config.player_count == 4 && optionvalue == 0)) {
              printk(KERN_INFO "recalboxrgbjamma: switch player_count to %d\n", (optionvalue == 1 ? 4 : 2));
              mutex_lock(&jamma_config.process_mutex);
              unregister_controllers();
              jamma_config.player_count = (optionvalue == 1 ? 4 : 2);
              register_controllers();
              mutex_unlock(&jamma_config.process_mutex);
            }
          } else if (strcmp(optionname, "options.jamma.controls.dualjoysticks") == 0) {
            if (jamma_config.dual_joy != optionvalue ) {
              printk(KERN_INFO "recalboxrgbjamma: switch dual_joy to %d\n", optionvalue);
              jamma_config.dual_joy = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.controls.debounce_time_ms") == 0) {
            if (jamma_config.debounce_time_ms != optionvalue ) {
              printk(KERN_INFO "recalboxrgbjamma: switch debounce_time_ms to %d\n", optionvalue);
              jamma_config.debounce_time_ms = optionvalue;
            }
          } else if (strcmp(optionname, "options.jamma.i2s") == 0) {
            if (jamma_config.i2s != optionvalue) {
              printk(KERN_INFO "recalboxrgbjamma: switch i2s to %d\n", optionvalue);
              jamma_config.i2s = optionvalue;
              pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_PI5_I2S], jamma_config.i2s);
            }
          } else if (strcmp(optionname, "options.jamma.videofilter") == 0) {
            if (jamma_config.videofilter != optionvalue && jamma_config.expander != NULL) {
              printk(KERN_INFO "recalboxrgbjamma: switch videofilter to %d\n", optionvalue);
              jamma_config.videofilter = optionvalue;
              pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_VIDEO_BYPASS], jamma_config.videofilter);
            }
          } else if (strcmp(optionname, "options.jamma.amp.boost") == 0) {
            if (jamma_config.amp_boost != optionvalue && jamma_config.expander != NULL) {
              printk(KERN_INFO "recalboxrgbjamma: switch amp_boost to %d\n", optionvalue);
              jamma_config.amp_boost = optionvalue;
              pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_GAIN0], jamma_config.amp_boost & 0x1);
              pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_GAIN1], jamma_config.amp_boost & 0x2);
            }
          } else if (strcmp(optionname, "options.jamma.amp.disable") == 0) {
            if (jamma_config.amp_disable != optionvalue && jamma_config.expander != NULL) {
              jamma_config.amp_disable = optionvalue;
              printk(KERN_INFO "recalboxrgbjamma: switch amp_disable to %d\n", optionvalue);
              pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_AMP_DISABLE], jamma_config.amp_disable & 0x1);
            }
          } else if (strcmp(optionname, "options.jamma.coincounter") == 0) {
            if (jamma_config.coin_counter != optionvalue && jamma_config.expander != NULL) {
              jamma_config.coin_counter = optionvalue;
              printk(KERN_INFO "recalboxrgbjamma: switch coin_counter to %d\n", optionvalue);
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
  chip->driver_data = (uintptr_t) i2c_get_match_data(client);
  if (!chip->driver_data)
    return -ENODEV;

  reg = devm_regulator_get(&client->dev, "vcc");
  if (IS_ERR(reg))
    return dev_err_probe(&client->dev, PTR_ERR(reg), "reg get err\n");

  ret = regulator_enable(reg);
  if (ret) {
    dev_err(&client->dev, "reg en err: %d\n", ret);
    return ret;
  }
  chip->regulator = reg;

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
  if (ret){
    dev_info(&client->dev, "We cannot talk to the chip, rgb jamma has been removed, so we set a flag to uninstall\n");
    jamma_config.unplugged = true;
    goto err_exit;
  }

  ret = pca953x_irq_setup(chip, irq_base);
  if (ret)
    goto err_exit;

  ret = devm_gpiochip_add_data(&client->dev, &chip->gpio_chip, chip);
  if (ret)
    goto err_exit;

  if (client->addr == 0x26) {
    jamma_config.expander = &chip->gpio_chip;
    unsigned int is_pi5 = 0;
    of_property_read_u32(client->dev.of_node, "is_pi5", &is_pi5);
    unsigned int rgbjamma2version = 0;
    of_property_read_u32(client->dev.of_node, "rev", &rgbjamma2version);
    jamma_config.version = rgbjamma2version ? (rgbjamma2version == 1 ? RRGBJ2PROTO1 : RRGBJ2REVA): RRGBJ1;
    if (is_pi5 == 1) {
      dev_info(&client->dev, "We are on RPi5, switching to i2s\n");
      jamma_config.i2s = true;
    }

    if(is_rrgbj2()) {
      dev_info(&client->dev, "RECALBOX RGB JAMMA 2 (%d)\n", jamma_config.version);
      dev_info(&client->dev, "Setting expander gpio coin counter pins (gpio %d and %d)\n", expander[jamma_config.version][EXP_CC_P1], expander[jamma_config.version][EXP_CC_P2]);
      pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_CC_P1], 0);
      pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_CC_P2], 0);
      dev_info(&client->dev, "Setting expander interupts (gpios %d, %d and %d)\n", expander[jamma_config.version][EXP_I2CINT1_GPIO], expander[jamma_config.version][EXP_I2CINT2_GPIO], expander[jamma_config.version][EXP_I2CINT3_GPIO]);
      if (pca953x_gpio_direction_input(&chip->gpio_chip, expander[jamma_config.version][EXP_I2CINT1_GPIO]) != 0)
        dev_err(&client->dev, "cannot set input for gpio EXP_I2CINT1_GPIO\n");
      if (pca953x_gpio_direction_input(&chip->gpio_chip, expander[jamma_config.version][EXP_I2CINT2_GPIO]) != 0)
        dev_err(&client->dev, "cannot set input for gpio EXP_I2CINT2_GPIO\n");
      if (pca953x_gpio_direction_input(&chip->gpio_chip, expander[jamma_config.version][EXP_I2CINT3_GPIO]) != 0)
        dev_err(&client->dev, "cannot set input for gpio EXP_I2CINT3_GPIO\n");
    }
    dev_info(&client->dev, "Setting expander gpio output pins\n");
    dev_info(&client->dev, "Setting EXP_I2S_FILTER on gpio %d to 0\n", expander[jamma_config.version][EXP_I2S_FILTER]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_I2S_FILTER], 0);
    dev_info(&client->dev, "Setting EXP_PI5_I2S on gpio %d to %d\n", expander[jamma_config.version][EXP_PI5_I2S], jamma_config.i2s);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_PI5_I2S], jamma_config.i2s);
    dev_info(&client->dev, "Setting EXP_VIDEO_BYPASS on gpio %d to 1\n", expander[jamma_config.version][EXP_VIDEO_BYPASS]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_VIDEO_BYPASS], 1);
    dev_info(&client->dev, "Setting EXP_AMP_DISABLE on gpio %d to 0\n", expander[jamma_config.version][EXP_AMP_DISABLE]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_AMP_DISABLE], 0);
    dev_info(&client->dev, "Setting EXP_GAIN1 on gpio %d to 0\n", expander[jamma_config.version][EXP_GAIN1]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_GAIN1], 0);
    dev_info(&client->dev, "Setting EXP_GAIN0 on gpio %d to 0\n", expander[jamma_config.version][EXP_GAIN0]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_GAIN0], 0);
    dev_info(&client->dev, "Setting EXP_FAN on gpio %d to 1\n", expander[jamma_config.version][EXP_FAN]);
    pca953x_gpio_direction_output(&chip->gpio_chip, expander[jamma_config.version][EXP_FAN], 1);
  } else {
    // We are a on control chip
    for (gpio_idx = 0; gpio_idx < 16; gpio_idx++) {
      if (pca953x_gpio_direction_input(&chip->gpio_chip, gpio_idx) != 0) {
        dev_err(&client->dev, "cannot set input for gpio %d on chip %d\n", gpio_idx, client->addr);
        goto err_exit;
      }
    }

    dev_info(&client->dev, "created jamma pca at %d\n", client->addr);
    if (client->addr == 0x21 || client->addr == 0x20 || client->addr == 0x25) {
      jamma_config.gpio_chip_0 = &chip->gpio_chip;
      if (process_inputs(&chip->gpio_chip) == 0) {
        if ((gpio_data & 0xFFFF) != 0xFFFF) {
          dev_info(&client->dev, "At module loading, some buttons are pressed on chip 0: %04X\n", (gpio_data & 0xFFFF));
        }
      }
    } else if (client->addr == 0x22 || client->addr == 0x24) {
      jamma_config.gpio_chip_1 = &chip->gpio_chip;
      if (process_inputs(&chip->gpio_chip) == 0) {
        if (((gpio_data >> 16) & 0xFFFF ) != 0xFFFF) {
          dev_info(&client->dev, "At module loading, some buttons are pressed on chip 1: %04X\n", (gpio_data >> 16) & 0xFFFF);
        }
        pca953x_gpio_direction_output(jamma_config.gpio_chip_1, 14, 0);
        pca953x_gpio_direction_output(jamma_config.gpio_chip_1, 15, 0);
        /* no need to auto detect pressed btn6 on jamma as it's configured by default on output gnd
         * and buttons on jamma are set to 5 by default
        if (PRESSED(gpio_data, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_6])) {
          dev_info(&client->dev, "disabled 6th button on jamma for player 1!\n");
          buttonsReleasedValues[buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_6]] = 0;
        }
        if (PRESSED(gpio_data, buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_6])) {
          dev_info(&client->dev, "disabled 6th button on jamma for player 2!\n");
          buttonsReleasedValues[buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_6]] = 0;
        }*/
      }
    } else if (client->addr == 0x27) {
      jamma_config.gpio_chip_2 = &chip->gpio_chip;
      if (process_inputs(&chip->gpio_chip) == 0) {
        if ((gpio_data >> 32) != 0xFFFF) {
          dev_info(&client->dev, "At module loading, some buttons are pressed on chip 2: %04X\n", (gpio_data >> 32));
        }
        if (PRESSED(gpio_data, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_SERVICE])) {
          dev_info(&client->dev, "reversing logic for SERVICE button on jamma!\n");
          buttonsReleasedValues[buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_SERVICE]] = 0;
        }
        if (PRESSED(gpio_data, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_TEST])) {
          dev_info(&client->dev, "reversing logic for TEST button on jamma!\n");
          buttonsReleasedValues[buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_TEST]] = 0;
        }
        if (PRESSED(gpio_data, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_COIN])) {
          dev_info(&client->dev, "reversing logic for P1 COIN button on jamma!\n");
          buttonsReleasedValues[buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_COIN]] = 0;
        }
        if (PRESSED(gpio_data, buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_COIN])) {
          dev_info(&client->dev, "reversing logic for P2 COIN button on jamma!\n");
          buttonsReleasedValues[buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_COIN]] = 0;
        }
      }
    }
  }

  if (jamma_config.gpio_chip_0 && jamma_config.gpio_chip_1 && jamma_config.gpio_chip_2 && jamma_config.expander) {
    printk(KERN_INFO
           "recalboxrgbjamma: all chips configured\n");
    int idx = 0;
    jamma_config.debounce_thread = kthread_create(process_debounce_and_turbo, &idx, "kthread_recalboxrgbjamma_debounce");
    printk(KERN_INFO
           "recalboxrgbjamma: setting debounce thread\n");
    if (jamma_config.debounce_thread != NULL) {
      wake_up_process(jamma_config.debounce_thread);
      printk(KERN_INFO
             "recalboxrgbjamma: kthread_recalboxrgbjamma_debounce is running\n");
    } else {
      printk(KERN_ERR
             "recalboxrgbjamma: kthread_recalboxrgbjamma_debounce could not be created\n");
      return -1;
    }

    jamma_config.config_thread = kthread_create(watch_configuration, &idx, "kthread_recalboxrgbjamma_cfg");
    printk(KERN_INFO
           "recalboxrgbjamma: setting configuration thread\n");
    if (jamma_config.config_thread != NULL) {
      wake_up_process(jamma_config.config_thread);
      printk(KERN_INFO
             "recalboxrgbjamma: kthread_recalboxrgbjamma_cfg is running\n");
    } else {
      printk(KERN_ERR
             "recalboxrgbjamma: kthread kthread_recalboxrgbjamma_cfg could not be created\n");
      return -1;
    }
  }
  if (is_rrgbj2()) {
    // Read once to reset interupts
    printk(KERN_INFO "recalboxrgbjamma: reseting interupts on expander\n");
    unsigned long gpio_values0 = 0xFFFF;
    pca953x_gpio_get_multiple(jamma_config.expander, &mask, &gpio_values0);
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
    {.compatible = "raspberrypi,recalboxrgbjamma", .data = (void *) (16 | PCA953X_TYPE | PCA_INT)},
    {}};

MODULE_DEVICE_TABLE(of, pca953x_dt_ids);
static SIMPLE_DEV_PM_OPS(pca953x_pm_ops, pca953x_suspend, pca953x_resume);

static void rrgbj_shutdown(struct i2c_client *client);

static struct i2c_driver pca953x_driver = {
    .driver = {
        .name = "recalboxrgbjamma",
        .pm = &pca953x_pm_ops,
        .of_match_table = pca953x_dt_ids,
        .acpi_match_table = pca953x_acpi_ids,
    },
    .probe = pca953x_probe,
    .remove = pca953x_remove,
    .shutdown = rrgbj_shutdown,
    .id_table = pca953x_id,
};


static ssize_t rgbjamma_plugged_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
  return sysfs_emit(buf, jamma_config.unplugged ? "false" : "true");
}

static struct kobj_attribute rgbjamma_plugged_attr = __ATTR(rgbjamma_plugged, 0444, rgbjamma_plugged_show, NULL);
static struct kobject *jamma_kobj;

static int __init pca953x_init(void) {
  int idx = 0;
  jamma_config.gpio_chip_0 = NULL;
  jamma_config.gpio_chip_1 = NULL;
  jamma_config.gpio_chip_2 = NULL;
  jamma_config.expander = NULL;

  jamma_config.hk_on_start = true;
  jamma_config.sound_on_start = true;
  jamma_config.credit_on_start_btn1 = true;
  jamma_config.exit_on_start = true;
  jamma_config.autofire = false;
  jamma_config.autofire_time = 50000000;// 50ms
  jamma_config.i2s = false;
  jamma_config.videofilter = false;
  jamma_config.buttons_on_jamma = 5;
  jamma_config.amp_disable = 0;
  jamma_config.amp_boost = 0;
  jamma_config.player_count = 2;
  jamma_config.dual_joy = false;
  jamma_config.debounce_time_ms = 10;
  jamma_config.coin_counter = true;
  jamma_config.unplugged = false;

  mutex_init(&jamma_config.process_mutex);
  printk(KERN_INFO "recalboxrgbjamma: registering controllers\n");
  mutex_lock(&jamma_config.process_mutex);
  register_controllers();
  mutex_unlock(&jamma_config.process_mutex);

  jamma_kobj = kobject_create_and_add("recalboxrgbjamma", kernel_kobj);
  if(!jamma_kobj) {
    printk(KERN_ERR "recalboxrgbjamma: unable to create /sys/kernel/recalboxrgbjamma\n");
  } else {
    if(sysfs_create_file(jamma_kobj, &rgbjamma_plugged_attr.attr)) {
      printk(KERN_ERR "recalboxrgbjamma: unable to create /sys/kernel/recalboxrgbjamma/rgbjamma_plugged\n");
      kobject_put(jamma_kobj);
    }
  }
  return i2c_add_driver(&pca953x_driver);
}

/* register after i2c/ postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(pca953x_init);

static void __exit pca953x_exit(void) {
  printk(KERN_INFO
         "recalboxrgbjamma: exiting\n");
  if(jamma_config.expander) {
    pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_FAN], 0);
  }

  if(jamma_kobj){
    sysfs_remove_file(jamma_kobj, &rgbjamma_plugged_attr.attr);
    kobject_put(jamma_kobj);
  }

  if (kthread_stop(jamma_config.config_thread)) {
    printk("recalboxrgbjamma: can't stop config thread");
  }
  if (kthread_stop(jamma_config.debounce_thread)) {
    printk("recalboxrgbjamma: can't stop debounce thread");
  }
  i2c_del_driver(&pca953x_driver);
  mutex_lock(&jamma_config.process_mutex);
  unregister_controllers();
  mutex_unlock(&jamma_config.process_mutex);
}

// fan off in process context: I2C can't run from an atomic sys_off handler
static void rrgbj_shutdown(struct i2c_client *client){
  if(jamma_config.expander)
    pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_FAN], 0);
}

module_exit(pca953x_exit);

MODULE_AUTHOR("eric miao <eric.miao@marvell.com>, digitalLumberjack <digitalLumberjack@recalbox.com>");
MODULE_DESCRIPTION("Recalbox RGB Jamma controllers driver");
MODULE_LICENSE("GPL");
