// SPDX-License-Identifier: GPL-2.0
/*
 * RECALBOX RGB JAMMA 2 Driver - Polling Version
 *
 * Driver for the Recalbox RGB JAMMA 2 arcade controller interface.
 * Supports up to 4 players via MCP GPIO expanders over I2C.
 *
 * This version uses polling instead of interrupts for maximum reliability.
 * I2C glitches are filtered by immediate re-read verification per chip.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#define DRIVER_NAME "recalboxrgbjamma2"
#define DRIVER_VERSION "3.2.1"

/* Polling interval in microseconds (1-2ms = 500-1000 Hz) */
#define POLL_INTERVAL_US_MIN 1000
#define POLL_INTERVAL_US_MAX 2000

#define DEBUG_ENABLED 0
#if DEBUG_ENABLED
  #define DBG(fmt, ...) pr_info(DRIVER_NAME ": " fmt, ##__VA_ARGS__)
#else
  #define DBG(fmt, ...) do {} while (0)
#endif

/* ============================================================================
 * MCP23017 Registers
 * ============================================================================ */

#define MCP23017_GPIOA    0x12

/* MCP I2C addresses */
#define MCP22_ADDR        0x22  /* Directions + START/COIN + B1 + specials */
#define MCP20_ADDR        0x20  /* Buttons B2-B6 + kick harness */

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MAX_PLAYERS           4
#define BUTTONS_PER_PLAYER    10
#define NUM_DIRECTIONS        4

/* MCP chip identifiers */
#define MCP_DIRECTIONS  0
#define MCP_BUTTONS     1
#define MCP_PLAYERS34   2
#define MCP_COUNT       3

static const u8 mcp_i2c_addr[MCP_COUNT] = {
  [MCP_DIRECTIONS] = MCP22_ADDR,
  [MCP_BUTTONS]    = MCP20_ADDR,
  [MCP_PLAYERS34]  = 0x23,
};

/* Direction indices */
enum direction {
  DIR_UP = 0,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT,
};

/* Button indices */
enum button_idx {
  BTN_IDX_1 = 0,
  BTN_IDX_2,
  BTN_IDX_3,
  BTN_IDX_4,
  BTN_IDX_5,
  BTN_IDX_6,
  BTN_IDX_START,
  BTN_IDX_COIN,
  BTN_IDX_SERVICE,
  BTN_IDX_TEST,
  BTN_IDX_HOTKEY,
  BTN_IDX_VOL_UP,
  BTN_IDX_VOL_DOWN,
  BTN_IDX_MAX,
};

#define LAST_GAME_BUTTON BTN_IDX_6

/* Output GPIO indices */
enum output_gpio {
  OUT_CC_P1 = 0,
  OUT_CC_P2,
  OUT_VIDEO_BYPASS,
  OUT_IS2_FILTER,
  OUT_AMP_DISABLE,
  OUT_GAIN0,
  OUT_GAIN1,
  OUT_FAN,
  OUT_B6GND,
  OUT_MAX,
};

/* ============================================================================
 * Data Structures
 * ============================================================================ */

struct output_config {
  const char *name;
  enum gpiod_flags default_value;
  struct gpio_desc *desc;
};

struct player_mapping {
  int directions[NUM_DIRECTIONS];
  int buttons[BUTTONS_PER_PLAYER];
  int kick[3];
};

struct rrgbj2_config {
  u32 buttons_on_jamma;
  u32 amp_boost;
  u32 player_count;
  u32 version;
  bool amp_disable;
  bool dual_joy;
  bool coin_counter;
  bool plugged;
};

struct rrgbj2_device {
  struct platform_device *pdev;
  struct rrgbj2_config config;

  /* Input devices */
  struct input_dev *player_devs[MAX_PLAYERS];

  /*
   * GPIO state: 48 bits, active-low (1 = released, 0 = pressed)
   *
   * Per-chip confirmed state is kept in mcp_confirmed[].
   * gpio_data is rebuilt from mcp_confirmed[] after each poll cycle.
   *
   * Glitch filtering: when a chip read differs from its confirmed state,
   * the chip is immediately re-read (~100us later). The change is accepted
   * only if both reads agree. This catches single-read I2C corruptions
   * without adding inter-cycle latency that would swallow fast button presses.
   */
  u64 gpio_data;
  u64 prev_gpio_data;

  /* Per-chip confirmed 16-bit state (after glitch filtering) */
  u16 mcp_confirmed[MCP_COUNT];

  /* I2C adapter for direct MCP access */
  struct i2c_adapter *i2c_adapter;

  /* Outputs */
  struct output_config outputs[OUT_MAX];

  /* Polling thread */
  struct task_struct *poll_thread;

  /* Config watch thread */
  struct task_struct *config_thread;

  /* Coin meter state */
  bool meter_done[2];
};

static struct rrgbj2_device *g_rrgbj2;

/* ============================================================================
 * Static Data
 * ============================================================================ */

static const char * const player_names[MAX_PLAYERS] = {
  "JammaControllerP1",
  "JammaControllerP2",
  "JammaControllerP3",
  "JammaControllerP4",
};

static const unsigned int button_codes[BTN_IDX_MAX] = {
  [BTN_IDX_1]       = BTN_A,
  [BTN_IDX_2]       = BTN_B,
  [BTN_IDX_3]       = BTN_X,
  [BTN_IDX_4]       = BTN_Y,
  [BTN_IDX_5]       = BTN_TL,
  [BTN_IDX_6]       = BTN_TR,
  [BTN_IDX_START]   = BTN_START,
  [BTN_IDX_COIN]    = BTN_SELECT,
  [BTN_IDX_SERVICE] = BTN_THUMBL,
  [BTN_IDX_TEST]    = BTN_THUMBR,
  [BTN_IDX_HOTKEY]  = BTN_MODE,
  [BTN_IDX_VOL_UP]  = KEY_VOLUMEUP,
  [BTN_IDX_VOL_DOWN]= KEY_VOLUMEDOWN,
};

static const struct player_mapping player_maps[MAX_PLAYERS] = {
  [0] = {
    .directions = {0, 1, 2, 3},
    .buttons = {4, 5, 6, 7, 8, 9, 10, 11, 15, 31},
    .kick = {12, 13, 14},
  },
  [1] = {
    .directions = {16, 17, 18, 19},
    .buttons = {20, 21, 22, 23, 24, 25, 26, 27, -1, -1},
    .kick = {28, 29, 30},
  },
  [2] = {
    .directions = {32, 14, 13, 12},
    .buttons = {28, 29, 37, 38, 39, -1, 40, 30, -1, -1},
    .kick = {-1, -1, -1},
  },
  [3] = {
    .directions = {33, 34, 35, 36},
    .buttons = {41, 42, 43, 44, 45, -1, 46, 47, -1, -1},
    .kick = {-1, -1, -1},
  },
};

static const struct output_config default_outputs[OUT_MAX] = {
  [OUT_CC_P1]        = {"CC_P1",        GPIOD_OUT_LOW,  NULL},
  [OUT_CC_P2]        = {"CC_P2",        GPIOD_OUT_LOW,  NULL},
  [OUT_VIDEO_BYPASS] = {"VIDEO_BYPASS", GPIOD_OUT_HIGH, NULL},
  [OUT_IS2_FILTER]   = {"IS2_FILTER",   GPIOD_OUT_LOW,  NULL},
  [OUT_AMP_DISABLE]  = {"AMP_DISABLE",  GPIOD_OUT_LOW,  NULL},
  [OUT_GAIN0]        = {"GAIN0",        GPIOD_OUT_LOW,  NULL},
  [OUT_GAIN1]        = {"GAIN1",        GPIOD_OUT_LOW,  NULL},
  [OUT_FAN]          = {"FAN",          GPIOD_OUT_HIGH, NULL},
  [OUT_B6GND]        = {"B6GND",        GPIOD_OUT_HIGH, NULL},
};

/* Pin-to-index maps per MCP chip */
static const int *mcp_pin_maps[MCP_COUNT];

/* MCP22 (0x22) pin mapping */
static const int mcp22_pin_to_idx[16] = {
  20,  /* Pin 0  = P2 B1 */
  4,   /* Pin 1  = P1 B1 */
  19,  /* Pin 2  = P2 RIGHT */
  3,   /* Pin 3  = P1 RIGHT */
  18,  /* Pin 4  = P2 LEFT */
  2,   /* Pin 5  = P1 LEFT */
  17,  /* Pin 6  = P2 DOWN */
  1,   /* Pin 7  = P1 DOWN */
  16,  /* Pin 8  = P2 UP */
  0,   /* Pin 9  = P1 UP */
  26,  /* Pin 10 = P2 START */
  10,  /* Pin 11 = P1 START */
  27,  /* Pin 12 = P2 COIN */
  11,  /* Pin 13 = P1 COIN */
  15,  /* Pin 14 = TEST */
  31,  /* Pin 15 = SERVICE */
};

/* MCP20 (0x20) pin mapping */
static const int mcp20_pin_to_idx[16] = {
  30,  /* Pin 0  = P2 B6K */
  29,  /* Pin 1  = P2 B5K */
  28,  /* Pin 2  = P2 B4K */
  14,  /* Pin 3  = P1 B6K */
  13,  /* Pin 4  = P1 B5K */
  12,  /* Pin 5  = P1 B4K */
  9,   /* Pin 6  = P1 B6 */
  25,  /* Pin 7  = P2 B6 */
  8,   /* Pin 8  = P1 B5 */
  7,   /* Pin 9  = P1 B4 */
  24,  /* Pin 10 = P2 B5 */
  23,  /* Pin 11 = P2 B4 */
  22,  /* Pin 12 = P2 B3 */
  6,   /* Pin 13 = P1 B3 */
  21,  /* Pin 14 = P2 B2 */
  5,   /* Pin 15 = P1 B2 */
};

/* MCP23 (0x23) pin mapping - Players 3 & 4 */
static const int mcp23_pin_to_idx[16] = {
  32,  /* Pin 0  = P3 UP */
  33,  /* Pin 1  = P4 UP */
  34,  /* Pin 2  = P4 DOWN */
  35,  /* Pin 3  = P4 LEFT */
  36,  /* Pin 4  = P4 RIGHT */
  37,  /* Pin 5  = P3 B3 */
  38,  /* Pin 6  = P3 B4 */
  39,  /* Pin 7  = P3 B5 */
  40,  /* Pin 8  = P3 START */
  41,  /* Pin 9  = P4 B1 */
  42,  /* Pin 10 = P4 B2 */
  43,  /* Pin 11 = P4 B3 */
  44,  /* Pin 12 = P4 B4 */
  45,  /* Pin 13 = P4 B5 */
  46,  /* Pin 14 = P4 START */
  47,  /* Pin 15 = P4 COIN */
};

/* Inversion masks - detected at startup */
static u16 mcp_invert_mask[MCP_COUNT] = {0, 0, 0};

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static inline void set_gpio_bit(u64 *data, int bit, int value)
{
  if (bit < 0)
    return;
  *data = (*data & ~(1ULL << bit)) | ((u64)!!value << bit);
}

static inline bool get_gpio_pressed(u64 data, int bit)
{
  return (bit >= 0) ? !((data >> bit) & 1) : false;
}

static int get_kick_bit(int player, int kick_idx)
{
  if (kick_idx < 0 || kick_idx > 2)
    return -1;
  return player_maps[player].kick[kick_idx];
}

/*
 * Rebuild gpio_data (48-bit) from per-chip confirmed states.
 */
static void rebuild_gpio_data(struct rrgbj2_device *dev)
{
  const int *pin_map;
  int mcp_id, i, idx;
  u16 state;
  u64 data = 0xFFFFFFFFFFFFULL;

  for (mcp_id = 0; mcp_id < MCP_COUNT; mcp_id++) {
    if (mcp_id == MCP_PLAYERS34 && dev->config.player_count != 4)
      continue;

    pin_map = mcp_pin_maps[mcp_id];
    state = dev->mcp_confirmed[mcp_id];

    for (i = 0; i < 16; i++) {
      idx = pin_map[i];
      if (idx >= 0)
        set_gpio_bit(&data, idx, (state >> i) & 1);
    }
  }

  dev->gpio_data = data;
}

/* ============================================================================
 * I2C Functions
 * ============================================================================ */

/*
 * Read GPIOA + GPIOB in a single sequential I2C transaction.
 * The MCP23017 auto-increments from GPIOA to GPIOB (IOCON.SEQOP=0 default),
 * giving an atomic 16-bit snapshot of both ports.
 */
static int mcp23017_read_all_gpios(struct i2c_adapter *adapter, u8 addr)
{
  struct i2c_msg msgs[2];
  u8 tx_buf = MCP23017_GPIOA;
  u8 rx_buf[2] = {0, 0};
  int ret;

  if (!adapter)
    return -ENODEV;

  msgs[0].addr = addr;
  msgs[0].flags = 0;
  msgs[0].len = 1;
  msgs[0].buf = &tx_buf;

  msgs[1].addr = addr;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = 2;
  msgs[1].buf = rx_buf;

  ret = i2c_transfer(adapter, msgs, 2);
  if (ret < 0)
    return ret;
  if (ret != 2)
    return -EIO;

  return rx_buf[0] | (rx_buf[1] << 8);
}

/* ============================================================================
 * GPIO Reading with Glitch Filtering
 * ============================================================================ */

static void detect_inverted_buttons(struct i2c_adapter *adapter)
{
  int state;

  if (!adapter)
    return;

  state = mcp23017_read_all_gpios(adapter, MCP22_ADDR);
  if (state >= 0) {
    mcp_invert_mask[MCP_DIRECTIONS] = (~state) & 0xFFFF;
    pr_info(DRIVER_NAME ": MCP22 initial=0x%04x invert_mask=0x%04x\n",
            state, mcp_invert_mask[MCP_DIRECTIONS]);
  }

  state = mcp23017_read_all_gpios(adapter, MCP20_ADDR);
  if (state >= 0) {
    mcp_invert_mask[MCP_BUTTONS] = (~state) & 0xFFFF;
    pr_info(DRIVER_NAME ": MCP20 initial=0x%04x invert_mask=0x%04x\n",
            state, mcp_invert_mask[MCP_BUTTONS]);
  }

  state = mcp23017_read_all_gpios(adapter, mcp_i2c_addr[MCP_PLAYERS34]);
  if (state >= 0) {
    mcp_invert_mask[MCP_PLAYERS34] = (~state) & 0xFFFF;
    pr_info(DRIVER_NAME ": MCP23 initial=0x%04x invert_mask=0x%04x\n",
            state, mcp_invert_mask[MCP_PLAYERS34]);
  }
}

/*
 * Poll all MCP chips with immediate re-read verification on change.
 *
 * For each chip:
 *   1. Read 16 bits
 *   2. If identical to confirmed state -> no change, move on
 *   3. If different -> immediately re-read the same chip (~100us later)
 *   4. Accept only the bits where both reads agree with each other
 *      (bits where they disagree keep their previous confirmed value)
 *
 * This filters single-read I2C glitches without adding inter-cycle delay,
 * so fast button presses are never swallowed.
 */
static void poll_mcp_chips(struct rrgbj2_device *dev)
{
  int mcp_id, raw1, raw2;
  u16 verified, disagree;

  if (!dev->i2c_adapter)
    return;

  for (mcp_id = 0; mcp_id < MCP_COUNT; mcp_id++) {
    /* Skip MCP23 if not in 4-player mode */
    if (mcp_id == MCP_PLAYERS34 && dev->config.player_count != 4)
      continue;

    raw1 = mcp23017_read_all_gpios(dev->i2c_adapter, mcp_i2c_addr[mcp_id]);
    if (raw1 < 0)
      continue;

    /* Apply inversion mask */
    raw1 ^= mcp_invert_mask[mcp_id];

    /* Fast path: no change from confirmed state */
    if ((u16)raw1 == dev->mcp_confirmed[mcp_id])
      continue;

    /* Change detected - immediate re-read to verify */
    raw2 = mcp23017_read_all_gpios(dev->i2c_adapter, mcp_i2c_addr[mcp_id]);
    if (raw2 < 0) {
      /* Re-read failed, keep confirmed state */
      continue;
    }

    raw2 ^= mcp_invert_mask[mcp_id];

    /*
     * Accept bits where both reads agree.
     * Where they disagree, keep the previous confirmed value.
     */
    disagree = (u16)(raw1 ^ raw2);

    if (disagree) {
      /* Mix: take agreed bits from raw1, keep confirmed for disagreed bits */
      verified = (raw1 & ~disagree) | (dev->mcp_confirmed[mcp_id] & disagree);
      DBG("MCP%d glitch filtered: raw1=0x%04x raw2=0x%04x disagree=0x%04x\n",
          mcp_id, (u16)raw1, (u16)raw2, disagree);
    } else {
      /* Both reads agree: accept entirely */
      verified = (u16)raw1;
    }

    dev->mcp_confirmed[mcp_id] = verified;
  }
}

/* ============================================================================
 * Input Reporting
 * ============================================================================ */

static void report_player_directions(struct rrgbj2_device *dev, int player)
{
  const struct player_mapping *map = &player_maps[player];
  u64 data = dev->gpio_data;
  struct input_dev *input;
  int abs_x, abs_y;
  int up, down, left, right;

  if (dev->config.dual_joy && player == 1) {
    input = dev->player_devs[0];
    abs_x = ABS_RX;
    abs_y = ABS_RY;
  } else {
    input = dev->player_devs[player];
    abs_x = ABS_X;
    abs_y = ABS_Y;
  }

  if (!input)
    return;

  up = get_gpio_pressed(data, map->directions[DIR_UP]);
  down = get_gpio_pressed(data, map->directions[DIR_DOWN]);
  left = get_gpio_pressed(data, map->directions[DIR_LEFT]);
  right = get_gpio_pressed(data, map->directions[DIR_RIGHT]);

  input_report_abs(input, abs_y, down - up);
  input_report_abs(input, abs_x, right - left);
}

static void report_player_buttons(struct rrgbj2_device *dev, int player)
{
  const struct player_mapping *map = &player_maps[player];
  struct input_dev *input = dev->player_devs[player];
  u64 data = dev->gpio_data;
  int btn_idx, bit, kick_bit;
  bool pressed;

  if (!input)
    return;

  for (btn_idx = 0; btn_idx < BUTTONS_PER_PLAYER; btn_idx++) {
    bit = map->buttons[btn_idx];

    if (dev->config.player_count == 4) {
      if (player <= 1 && btn_idx <= LAST_GAME_BUTTON &&
          btn_idx >= dev->config.buttons_on_jamma)
        continue;
      pressed = get_gpio_pressed(data, bit);
    } else {
      if (btn_idx <= LAST_GAME_BUTTON && btn_idx >= dev->config.buttons_on_jamma) {
        kick_bit = get_kick_bit(player, btn_idx - 3);
        pressed = get_gpio_pressed(data, kick_bit);
      } else {
        kick_bit = (btn_idx >= 3 && btn_idx <= 5) ? get_kick_bit(player, btn_idx - 3) : -1;
        pressed = get_gpio_pressed(data, bit) || get_gpio_pressed(data, kick_bit);
      }
    }

    input_report_key(input, button_codes[btn_idx], pressed);
  }
}

static void process_inputs(struct rrgbj2_device *dev)
{
  int player;

  for (player = 0; player < dev->config.player_count; player++) {
    if (!dev->player_devs[player])
      continue;

    report_player_directions(dev, player);
    report_player_buttons(dev, player);
    input_sync(dev->player_devs[player]);
  }
}

/* ============================================================================
 * Coin Meters
 * ============================================================================ */

static void trigger_coin_meter(struct rrgbj2_device *dev, int player)
{
  enum output_gpio out = (player == 0) ? OUT_CC_P1 : OUT_CC_P2;

  if (!dev->outputs[out].desc)
    return;

  pr_info(DRIVER_NAME ": triggering coin meter P%d\n", player + 1);
  gpiod_set_value_cansleep(dev->outputs[out].desc, 1);
  usleep_range(100000, 150000);
  gpiod_set_value_cansleep(dev->outputs[out].desc, 0);
}

static void manage_coin_meters(struct rrgbj2_device *dev)
{
  int player;
  int coin_bit;
  bool pressed;

  if (!dev->config.coin_counter)
    return;

  for (player = 0; player < 2; player++) {
    coin_bit = player_maps[player].buttons[BTN_IDX_COIN];
    pressed = get_gpio_pressed(dev->gpio_data, coin_bit);

    if (pressed && !dev->meter_done[player]) {
      trigger_coin_meter(dev, player);
      dev->meter_done[player] = true;
    } else if (!pressed) {
      dev->meter_done[player] = false;
    }
  }
}

/* ============================================================================
 * Polling Thread
 * ============================================================================ */

static int poll_thread_fn(void *data)
{
  struct rrgbj2_device *dev = data;

  pr_info(DRIVER_NAME ": Polling thread started (%d-%d us interval)\n",
          POLL_INTERVAL_US_MIN, POLL_INTERVAL_US_MAX);

  while (!kthread_should_stop()) {
    /* Read all MCP chips with immediate re-read verification */
    poll_mcp_chips(dev);

    /* Rebuild 48-bit gpio_data from per-chip confirmed states */
    rebuild_gpio_data(dev);

    /* Only process if confirmed state changed */
    if (dev->gpio_data != dev->prev_gpio_data) {
      DBG("State changed: 0x%012llx -> 0x%012llx\n",
          dev->prev_gpio_data, dev->gpio_data);

      process_inputs(dev);
      manage_coin_meters(dev);

      dev->prev_gpio_data = dev->gpio_data;
    }

    /* Sleep - allows CPU to do other things */
    usleep_range(POLL_INTERVAL_US_MIN, POLL_INTERVAL_US_MAX);
  }

  pr_info(DRIVER_NAME ": Polling thread stopped\n");
  return 0;
}

/* ============================================================================
 * Input Device Registration
 * ============================================================================ */

static struct input_dev *create_input_device(struct rrgbj2_device *dev, int player)
{
  struct input_dev *input;
  int i, ret;

  input = input_allocate_device();
  if (!input)
    return NULL;

  input->name = player_names[player];
  input->phys = player_names[player];
  input->id.bustype = BUS_I2C;
  input->id.vendor = 0x7262;
  input->id.product = 0x0A01;
  input->id.version = 0x0100;

  input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

  input_set_abs_params(input, ABS_X, -1, 1, 0, 0);
  input_set_abs_params(input, ABS_Y, -1, 1, 0, 0);
  input_set_abs_params(input, ABS_RX, -1, 1, 0, 0);
  input_set_abs_params(input, ABS_RY, -1, 1, 0, 0);

  for (i = 0; i < BTN_IDX_MAX; i++)
    __set_bit(button_codes[i], input->keybit);

  ret = input_register_device(input);
  if (ret) {
    input_free_device(input);
    return NULL;
  }

  pr_info(DRIVER_NAME ": Registered controller for player %d\n", player + 1);
  return input;
}

static int register_controllers(struct rrgbj2_device *dev)
{
  int i;

  for (i = 0; i < dev->config.player_count; i++) {
    dev->player_devs[i] = create_input_device(dev, i);
    if (!dev->player_devs[i])
      return -ENOMEM;
  }

  return 0;
}

static void unregister_controllers(struct rrgbj2_device *dev)
{
  int i;

  for (i = 0; i < MAX_PLAYERS; i++) {
    if (dev->player_devs[i]) {
      input_unregister_device(dev->player_devs[i]);
      dev->player_devs[i] = NULL;
    }
  }
}

/* ============================================================================
 * Output GPIOs
 * ============================================================================ */

static int setup_output_gpios(struct rrgbj2_device *dev)
{
  struct platform_device *pdev = dev->pdev;
  int i;

  memcpy(dev->outputs, default_outputs, sizeof(default_outputs));

  for (i = 0; i < OUT_MAX; i++) {
    dev->outputs[i].desc = devm_gpiod_get_index(&pdev->dev, "configs", i,
                                                dev->outputs[i].default_value);
    if (IS_ERR(dev->outputs[i].desc)) {
      dev_warn(&pdev->dev, "Failed to get output GPIO %s\n", dev->outputs[i].name);
      dev->outputs[i].desc = NULL;
    }
  }

  return 0;
}

/* ============================================================================
 * Configuration File Parser
 * ============================================================================ */

#define CONFIG_PATH "/boot/crt/recalbox-crt-options.cfg"
#define READ_BUF_SIZE 2048
#define LINE_BUF_SIZE 256

struct config_handler {
  const char *option;
  void (*apply)(struct rrgbj2_device *dev, int value);
};

static void apply_buttons_on_jamma(struct rrgbj2_device *dev, int value)
{
  if (dev->config.buttons_on_jamma == value)
    return;

  pr_info(DRIVER_NAME ": buttons_on_jamma = %d\n", value);
  dev->config.buttons_on_jamma = value;

  if (dev->outputs[OUT_B6GND].desc)
    gpiod_set_value_cansleep(dev->outputs[OUT_B6GND].desc, value < 6);
}

static void apply_4players(struct rrgbj2_device *dev, int value)
{
  int new_count = value ? 4 : 2;

  if (dev->config.player_count == new_count)
    return;

  pr_info(DRIVER_NAME ": player_count = %d\n", new_count);

  unregister_controllers(dev);
  dev->config.player_count = new_count;
  register_controllers(dev);
}

static void apply_dual_joy(struct rrgbj2_device *dev, int value)
{
  if (dev->config.dual_joy == !!value)
    return;

  pr_info(DRIVER_NAME ": dual_joy = %d\n", value);
  dev->config.dual_joy = !!value;
}

static void apply_amp_boost(struct rrgbj2_device *dev, int value)
{
  if (dev->config.amp_boost == value)
    return;

  pr_info(DRIVER_NAME ": amp_boost = %d\n", value);
  dev->config.amp_boost = value;

  if (dev->outputs[OUT_GAIN0].desc)
    gpiod_set_value_cansleep(dev->outputs[OUT_GAIN0].desc, value & 1);
  if (dev->outputs[OUT_GAIN1].desc)
    gpiod_set_value_cansleep(dev->outputs[OUT_GAIN1].desc, value & 1);
}

static void apply_amp_disable(struct rrgbj2_device *dev, int value)
{
  if (dev->config.amp_disable == !!value)
    return;

  pr_info(DRIVER_NAME ": amp_disable = %d\n", value);
  dev->config.amp_disable = !!value;

  if (dev->outputs[OUT_AMP_DISABLE].desc)
    gpiod_set_value_cansleep(dev->outputs[OUT_AMP_DISABLE].desc, value & 1);
}

static void apply_coin_counter(struct rrgbj2_device *dev, int value)
{
  if (dev->config.coin_counter == !!value)
    return;

  pr_info(DRIVER_NAME ": coin_counter = %d\n", value);
  dev->config.coin_counter = !!value;
}

static const struct config_handler config_handlers[] = {
  {"options.jamma.controls.buttons_on_jamma", apply_buttons_on_jamma},
  {"options.jamma.controls.4players",         apply_4players},
  {"options.jamma.controls.dualjoysticks",    apply_dual_joy},
  {"options.jamma.amp.boost",                 apply_amp_boost},
  {"options.jamma.amp.disable",               apply_amp_disable},
  {"options.jamma.coincounter",               apply_coin_counter},
  {NULL, NULL},
};

static void parse_config_line(struct rrgbj2_device *dev, const char *line)
{
  char option[LINE_BUF_SIZE];
  int value, i;

  if (sscanf(line, "%s = %d", option, &value) != 2)
    return;

  for (i = 0; config_handlers[i].option; i++) {
    if (strcmp(option, config_handlers[i].option) == 0) {
      config_handlers[i].apply(dev, value);
      break;
    }
  }
}

static int load_config(struct rrgbj2_device *dev)
{
  struct file *fp;
  char *buf, *line, *ptr, *end;
  ssize_t size;
  loff_t pos = 0;

  fp = filp_open(CONFIG_PATH, O_RDONLY, 0);
  if (IS_ERR(fp))
    return 0;

  buf = kmalloc(READ_BUF_SIZE, GFP_KERNEL);
  line = kmalloc(LINE_BUF_SIZE, GFP_KERNEL);
  if (!buf || !line) {
    kfree(buf);
    kfree(line);
    filp_close(fp, NULL);
    return -ENOMEM;
  }

  size = kernel_read(fp, buf, READ_BUF_SIZE - 1, &pos);
  filp_close(fp, NULL);

  if (size <= 0)
    goto out;

  buf[size] = '\0';
  ptr = buf;

  while ((end = strchr(ptr, '\n')) != NULL) {
    *end = '\0';
    if (*ptr != '#' && *ptr != '\0') {
      strncpy(line, ptr, LINE_BUF_SIZE - 1);
      line[LINE_BUF_SIZE - 1] = '\0';
      parse_config_line(dev, line);
    }
    ptr = end + 1;
  }

  if (*ptr != '#' && *ptr != '\0')
    parse_config_line(dev, ptr);

  out:
  kfree(buf);
  kfree(line);
  return 0;
}

static int config_thread_fn(void *data)
{
  struct rrgbj2_device *dev = data;

  while (!kthread_should_stop()) {
    load_config(dev);
    usleep_range(2000000, 5000000);
  }

  return 0;
}

/* ============================================================================
 * Sysfs Interface
 * ============================================================================ */

static struct kobject *sysfs_kobj;

static ssize_t plugged_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  if (!g_rrgbj2)
    return sysfs_emit(buf, "false");
  return sysfs_emit(buf, g_rrgbj2->config.plugged ? "true" : "false");
}

static struct kobj_attribute plugged_attr = __ATTR_RO(plugged);

static int setup_sysfs(void)
{
  int ret;

  sysfs_kobj = kobject_create_and_add(DRIVER_NAME, kernel_kobj);
  if (!sysfs_kobj)
    return -ENOMEM;

  ret = sysfs_create_file(sysfs_kobj, &plugged_attr.attr);
  if (ret) {
    kobject_put(sysfs_kobj);
    sysfs_kobj = NULL;
    return ret;
  }

  return 0;
}

static void cleanup_sysfs(void)
{
  if (sysfs_kobj) {
    sysfs_remove_file(sysfs_kobj, &plugged_attr.attr);
    kobject_put(sysfs_kobj);
    sysfs_kobj = NULL;
  }
}

/* ============================================================================
 * Platform Driver
 * ============================================================================ */

static int rrgbj2_probe(struct platform_device *pdev)
{
  struct rrgbj2_device *dev;
  int ret;

  dev_info(&pdev->dev, "Probing " DRIVER_NAME " (polling version)\n");

  dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
  if (!dev)
    return -ENOMEM;

  dev->pdev = pdev;
  dev->gpio_data = 0xFFFFFFFFFFFFULL;
  dev->prev_gpio_data = 0xFFFFFFFFFFFFULL;

  /* Init per-chip confirmed state to all-high (all released) */
  dev->mcp_confirmed[MCP_DIRECTIONS] = 0xFFFF;
  dev->mcp_confirmed[MCP_BUTTONS]    = 0xFFFF;
  dev->mcp_confirmed[MCP_PLAYERS34]  = 0xFFFF;

  /* Init pin map lookup table */
  mcp_pin_maps[MCP_DIRECTIONS] = mcp22_pin_to_idx;
  mcp_pin_maps[MCP_BUTTONS]    = mcp20_pin_to_idx;
  mcp_pin_maps[MCP_PLAYERS34]  = mcp23_pin_to_idx;

  /* Default configuration */
  dev->config.buttons_on_jamma = 5;
  dev->config.player_count = 2;
  dev->config.coin_counter = true;

  g_rrgbj2 = dev;

  /* Setup output GPIOs */
  ret = setup_output_gpios(dev);
  if (ret) {
    g_rrgbj2 = NULL;
    return ret;
  }

  /* Get I2C adapter */
  dev->i2c_adapter = i2c_get_adapter(1);
  if (!dev->i2c_adapter) {
    dev_err(&pdev->dev, "Failed to get I2C adapter\n");
    g_rrgbj2 = NULL;
    return -ENODEV;
  }

  dev_info(&pdev->dev, "Using I2C adapter for MCP access\n");

  /* Release B6 from ground so detect_inverted_buttons reads the true idle state. */
  if (dev->outputs[OUT_B6GND].desc) {
    gpiod_set_value_cansleep(dev->outputs[OUT_B6GND].desc, 0);
    msleep(5);
  }

  /* Detect inverted buttons */
  detect_inverted_buttons(dev->i2c_adapter);

  /* Restore B6GND to match the default buttons_on_jamma config. */
  if (dev->outputs[OUT_B6GND].desc)
    gpiod_set_value_cansleep(dev->outputs[OUT_B6GND].desc, dev->config.buttons_on_jamma < 6);

  /* Register input devices */
  ret = register_controllers(dev);
  if (ret)
    goto err_controllers;

  /* Start polling thread */
  dev->poll_thread = kthread_run(poll_thread_fn, dev, "rrgbj2_poll");
  if (IS_ERR(dev->poll_thread)) {
    ret = PTR_ERR(dev->poll_thread);
    goto err_poll;
  }

  /* Start config thread */
  dev->config_thread = kthread_run(config_thread_fn, dev, "rrgbj2_cfg");
  if (IS_ERR(dev->config_thread)) {
    ret = PTR_ERR(dev->config_thread);
    goto err_config;
  }

  of_property_read_u32(pdev->dev.of_node, "rev", &dev->config.version);
  dev->config.plugged = true;

  platform_set_drvdata(pdev, dev);

  dev_info(&pdev->dev, "RECALBOX RGB JAMMA 2 v%d initialized (polling + verify-on-change)\n",
           dev->config.version);
  return 0;

  err_config:
  kthread_stop(dev->poll_thread);
  err_poll:
  unregister_controllers(dev);
  err_controllers:
  i2c_put_adapter(dev->i2c_adapter);
  g_rrgbj2 = NULL;
  return ret;
}

static void rrgbj2_remove(struct platform_device *pdev)
{
  struct rrgbj2_device *dev = platform_get_drvdata(pdev);

  dev_info(&pdev->dev, "Removing " DRIVER_NAME "\n");

  if (dev->config_thread)
    kthread_stop(dev->config_thread);

  if (dev->poll_thread)
    kthread_stop(dev->poll_thread);

  unregister_controllers(dev);

  if (dev->i2c_adapter)
    i2c_put_adapter(dev->i2c_adapter);

  g_rrgbj2 = NULL;
}

static const struct of_device_id rrgbj2_of_match[] = {
  { .compatible = "raspberrypi,recalboxrgbjamma2mcp" },
  { },
};
MODULE_DEVICE_TABLE(of, rrgbj2_of_match);

static void rrgbj2_shutdown(struct platform_device *pdev);

static struct platform_driver rrgbj2_driver = {
  .probe = rrgbj2_probe,
  .remove = rrgbj2_remove,
  .shutdown = rrgbj2_shutdown,
  .driver = {
    .name = DRIVER_NAME "mcp",
    .of_match_table = rrgbj2_of_match,
  },
};

// fan off in process context: I2C can't run from an atomic sys_off handler
static void rrgbj2_shutdown(struct platform_device *pdev)
{
  if (g_rrgbj2 && g_rrgbj2->outputs[OUT_FAN].desc)
    gpiod_set_value_cansleep(g_rrgbj2->outputs[OUT_FAN].desc, 0);
}

/* ============================================================================
 * Module Init/Exit
 * ============================================================================ */

static int __init rrgbj2_init(void)
{
  int ret;

  pr_info(DRIVER_NAME ": === MODULE INIT === v%s (polling+verify) DEBUG=%d\n",
          DRIVER_VERSION, DEBUG_ENABLED);

  ret = platform_driver_register(&rrgbj2_driver);
  if (ret)
    return ret;

  setup_sysfs();

  pr_info(DRIVER_NAME ": === MODULE INIT DONE ===\n");

  return 0;
}

static void __exit rrgbj2_exit(void)
{
  cleanup_sysfs();
  platform_driver_unregister(&rrgbj2_driver);
  pr_info(DRIVER_NAME ": Module unloaded\n");
}

module_init(rrgbj2_init);
module_exit(rrgbj2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Recalbox Team");
MODULE_DESCRIPTION("RECALBOX RGB JAMMA 2 Controller Driver - Polling Version");
MODULE_VERSION(DRIVER_VERSION);