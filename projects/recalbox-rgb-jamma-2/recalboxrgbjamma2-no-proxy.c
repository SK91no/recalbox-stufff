#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/dev_printk.h>
#include <linux/input.h>
#include <asm-generic/bitops/non-atomic.h>

#include <linux/bitmap.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/reboot.h>
#include <linux/kthread.h>

#include <linux/unaligned.h>

// Interupts
#include <linux/dmi.h>

#define DEBUG 1

static struct config {
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

enum GpioButton {
  J_UP, J_DOWN, J_LEFT, J_RIGHT,
  J_B1, J_B2,  J_B3,
  J_B4,  J_B5,  J_B6,
  J_B4K, J_B5K, J_B6K,
  J_START, J_COIN,
  J_BTN_MAX
};

struct button_data {
  struct gpio_desc * gpio;
  bool pressed;
  unsigned int data_position;
  enum GpioButton gpio_btn;
  uint8_t data_index;
};
#define MAX_PLAYERS 4

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

// Standard buttons mapping
#define BTN_PER_PLAYER 13
#define BTN_PER_PLAYER_ON_JAMMA 10
#define TOTAL_GPIO_ON_PCA 48

#define P1_START 43
#define P1_SERVICE buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_SERVICE]
#define P1_TEST buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_TEST]

#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

struct button_data buttons[TOTAL_GPIO_ON_PCA] = {0};

static const char *controllers_names[MAX_PLAYERS] = {"JammaControllerP1", "JammaControllerP2", "JammaControllerP3", "JammaControllerP4"};
struct input_dev * player_devs[MAX_PLAYERS] = {NULL,NULL,NULL,NULL};

static const int direction_bits[MAX_PLAYERS][4] = {
  // Player 1
  {0, 1, 3, 4},
  {16, 17, 18, 19},
    // TODO
  {21, 26, 25, 24},
  {3, 4, 5, 6},
};

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

//// GpioButton to input code
//static const int gpio_to_btn_code[J_BTN_MAX] = {
//    // Directions
//    -1, -1, -1, -1,
//    // J_B1, J_B2,  J_B3,
//    BTN_A, BTN_B, BTN_X,
//    // J_B4,  J_B5,  J_B6,
//    BTN_Y, BTN_TL, BTN_TR,
//    // J_B4K, J_B5K, J_B6K,
//    BTN_Y, BTN_TL, BTN_TR,
//    // J_START, J_COIN,
//    BTN_START, BTN_SELECT
//};

static int buttons_bits[MAX_PLAYERS][2][BTN_PER_PLAYER_ON_JAMMA] = {
  // Player 1
  {
    // The value is the bit of the data containing the
    // information for the button at the same index in buttons_codes array
    // Start is a special case
    {4, 5, 6, 7, 8, 9, -1, 14, 15, 31},
    // Kick harness buttons mapping
    // Only use 3 btns here
    {-1, -1, -1, 10, 11, 12, -1, -1, -1, -1},
  },
  // Player 2
  {
      {20, 21, 22, 23, 24, 25, 29, 30, -1, -1},
      {-1, -1, -1, 26, 27, 28, -1, -1, -1, -1},
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
  }
};


//static void setPlayerButtonState(uint player, enum Button button, bool pressed)
//{
//
//
//}

//static irqreturn_t rrgbj2_irq_isr(int irq, void *dev_id)
//{
//  struct button_data *bdata = dev_id;
//  struct input_dev *input = bdata->input;
//  printk(KERN_INFO "interupt on button %d for player %d\n", bdata->gpio_btn, bdata->player);
//  int state = gpiod_get_value_cansleep(bdata->gpio);
//  if (state < 0) {
//    dev_err(input->dev.parent, "failed to get gpio state: %d\n", state);
//    return IRQ_HANDLED;
//  }
////
////  setPlayerButtonState(bdata->player, bdata->gpio_btn, state);
//  if(bdata->gpio_btn == J_UP)
//    input_report_abs(input, ABS_RY, state * -1);
//  else if(bdata->gpio_btn == J_DOWN)
//    input_report_abs(input, ABS_RY, state);
//  else if(bdata->gpio_btn == J_LEFT)
//    input_report_abs(input, ABS_RX, state);
//  else if(bdata->gpio_btn == J_RIGHT)
//    input_report_abs(input, ABS_RX, state);
//  else {
//    printk(KERN_INFO "reporting gpio %d state %d, code %x\n", bdata->gpio_btn, state, gpio_to_btn_code[bdata->gpio_btn]);
//    input_report_key(input, gpio_to_btn_code[bdata->gpio_btn], state);
//  }
//  input_sync(input);
//  return IRQ_HANDLED;
//}

static u_int64_t gpio_data = 0xFFFFFFFFFFFF;

// Version with direct report
//static irqreturn_t rrgbj2_irq_isr_manage(struct button_data *bdata)
//{
//  struct input_dev *input = bdata->input;
//
//  if(bdata->gpio_btn == J_UP)
//    input_report_abs(input, ABS_Y, bdata->pressed ? -1 : 0);
//  else if(bdata->gpio_btn == J_DOWN)
//    input_report_abs(input, ABS_Y, bdata->pressed);
//  else if(bdata->gpio_btn == J_LEFT)
//    input_report_abs(input, ABS_X, bdata->pressed ? -1 : 0);
//  else if(bdata->gpio_btn == J_RIGHT)
//    input_report_abs(input, ABS_X, bdata->pressed);
//  else {
//    printk(KERN_INFO "reporting gpio %d state %d, code %x\n", bdata->gpio_btn, bdata->pressed, gpio_to_btn_code[bdata->gpio_btn]);
//    input_report_key(input, gpio_to_btn_code[bdata->gpio_btn], bdata->pressed);
//  }
//  input_sync(input);
//  return IRQ_HANDLED;
//}

static u_int64_t lastGpioState = 0;
static u_int64_t debounceGpioValue = 0;
static long long int lastActionOn[TOTAL_GPIO_ON_PCA] = {0};

#define SETGPIO(data_addr, gpio, value) (*(data_addr) = ((*(data_addr) & (~(1ULL << (gpio)))) | ((value) << (gpio))))

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


static bool meterdone1 = false;
static bool meterdone2 = false;
static void manage_coin_meters(u_int64_t *data_chips) {
  // Coin conter
  if(jamma_config.coin_counter){
    if(PRESSED(*data_chips, buttons_bits[PLAYER1][JAMMA_BTNS][JAMMA_BTN_COIN]) && !meterdone1) {
      printk(KERN_INFO "recalboxrgbjamma: manage_coin_meters: triggering coin meter P1\n");
      // TODO pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P1], 1);
      usleep_range(100000, 150000);
      // TODO pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P1], 0);
      meterdone1 = true;
    } else {
      meterdone1 = false;
    }
    if(PRESSED(*data_chips, buttons_bits[PLAYER2][JAMMA_BTNS][JAMMA_BTN_COIN]) && !meterdone2) {
      printk(KERN_INFO "recalboxrgbjamma: manage_coin_meters: triggering coin meter P2\n");
      // TODO pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P2], 1);
      usleep_range(100000, 150000);
      // TODO pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_CC_P2], 0);
      meterdone2 = true;
    } else {
      meterdone2 = false;
    }
  }
}


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

static int process_inputs(void) {
  u_int64_t debounced_data = 0;
  u_int64_t gpio_data_chip = 0;
  long long int time_start = ktime_to_ns(ktime_get_boottime());
  DEBUG &&printk(KERN_INFO "recalboxrgbjamma2: process_inputs: locking mutex\n");
  mutex_lock(&jamma_config.process_mutex);
  for (int i = PLAYER1; i < jamma_config.player_count; i++) {
    if (player_devs[i] == NULL) {
      printk(KERN_INFO "recalboxrgbjamma2: process_inputs: some players are NULL, unable to process inputs\n");
      mutex_unlock(&jamma_config.process_mutex);
      return 0;
    }
  }
  DEBUG && printk(KERN_INFO "recalboxrgbjamma2: process_inputs: debounce mutex\n");
  debounce(&gpio_data, &debounced_data, &time_start);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma2: process_inputs: input_report\n");
  input_report(&debounced_data, &time_start);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma2: process_inputs: manage_coin_meters\n");
  manage_coin_meters(&debounced_data);
  DEBUG && printk(KERN_INFO "recalboxrgbjamma2: process_inputs: unlocking mutex\n");
  mutex_unlock(&jamma_config.process_mutex);
  return 0;
}


static irqreturn_t rrgbj2_irq_isr_manage(struct button_data *bdata)
{
  static u_int64_t gpiodatamask = 0xFFFF000000000000;
  // For P1 and P2, they only have one chip

  SETGPIO(&gpio_data, bdata->data_index, (!bdata->pressed));
  printk(KERN_INFO "recalboxrgbjamma2: setting gpio_data to 0x%lx\n", gpio_data);
  process_inputs();
  return IRQ_HANDLED;
}

//static irqreturn_t rrgbj2_irq_isr_rise(int irq, void *dev_id)
//{
//  struct button_data *bdata = dev_id;
//  bdata->pressed = false;
//  return rrgbj2_irq_isr_manage(dev_id);
//}
//static irqreturn_t rrgbj2_irq_isr_fall(int irq, void *dev_id)
//{
//  struct button_data *bdata = dev_id;
//  bdata->pressed = true;
//  return rrgbj2_irq_isr_manage(dev_id);
//}
static irqreturn_t rrgbj2_irq_isr_switch(int irq, void *dev_id)
{
  struct button_data *bdata = dev_id;
  printk(KERN_INFO "recalboxrgbjamma2: interupt on button %d \n", bdata->data_index);
  bdata->pressed = !gpiod_get_value_cansleep(bdata->gpio);
  printk(KERN_INFO "recalboxrgbjamma2: button %d is %s\n", bdata->data_index, bdata->pressed ? "pressed" : "released");

  return rrgbj2_irq_isr_manage(dev_id);
}


static struct input_dev * register_controller(int player) {
  struct input_dev *player_dev;
  int err = 0;

  if(!(player_dev = input_allocate_device())){
    pr_err("recalboxrgbjamma: Cannot allocate input device\n");
    return NULL;
  }
  printk(KERN_INFO "recalboxrgbjamma: register player %d controller\n", player);

  player_dev->name = controllers_names[player];
  player_dev->phys = controllers_names[player];
  player_dev->id.bustype = BUS_I2C;
  player_dev->id.vendor = 0x7262;
  player_dev->id.product = 0x0A01;
  player_dev->id.version = 0x0100;

  player_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

  for (int i = 0; i < 2; i++) {
    input_set_abs_params(player_dev, ABS_X + i, -1, 1, 0, 0);
    input_set_abs_params(player_dev, ABS_RX + i, -1, 1, 0, 0);
  }
  for (int i = 0; i < BTN_PER_PLAYER; i++)
    __set_bit(buttons_codes[i], player_dev->keybit);

  if ((err = input_register_device(player_dev))) {
    pr_err("recalboxrgbjamma: Cannot register input device\n");
    input_free_device(player_dev);
    return NULL;
  }
  return player_dev;
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



static int create_button_interupt(struct platform_device *pdev, const char * gpioName, int button_index, int data_index){
    struct gpio_desc *desc = devm_gpiod_get_index(&(pdev->dev), gpioName, button_index, GPIOD_IN);
    if(!desc){
      dev_err(&pdev->dev, "unable to get gpio %d for gpioName %s\n", button_index, gpioName);
      return -1;
    }
    buttons[data_index].gpio = desc;
    buttons[data_index].gpio_btn = button_index;
    buttons[data_index].data_index = data_index;
    buttons[data_index].pressed = gpiod_get_value_cansleep(desc);

    // Interupts
    int irq = gpiod_to_irq(desc);
    int error;
    if (irq < 0) {
      error = irq;
      dev_err_probe(&pdev->dev, error,
              "Unable to get irq number for GPIO %d on gpioName %s\n",button_index, gpioName);
      return -1;
    }
    error = devm_request_any_context_irq(&pdev->dev, irq, rrgbj2_irq_isr_switch, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "rrgbj2", &buttons[data_index]);
    if(error < 0) {
      dev_err_probe(&pdev->dev, error,
                      "Unable to devm_request_any_context_irq IRQF_TRIGGER_RISING for GPIO %d on gpioName %s\n",button_index, gpioName);
      return -1;
    }
    return 0;
}


static int rrgbj2_probe(struct platform_device *pdev) {
  dev_info(&pdev->dev, "rrgbj2_probe\n");
  int err = 0;
  // Two first players, each one on a specific mcp
  for(int player = 0; player < 2; player++) {
    const char * gpioName = player == 0 ? "player1" : "player2";
    // Register controller
    player_devs[player] = register_controller(player);
    if(!player_devs[player]) {
      pr_err("recalboxrgbjamma: Cannot register input device\n");
      goto err;
    }
    for(int button = 0; button < J_BTN_MAX; button++) {
      err = create_button_interupt(pdev, gpioName, button, button*(player +1));
      if(err < 0) goto err;
    }

  }
  // TEST
  if(create_button_interupt(pdev, "specials", 0, 15) < 0) goto err;
  if(create_button_interupt(pdev, "specials", 1, 31) < 0) goto err;


  of_property_read_u32(pdev->dev.of_node, "rev", &jamma_config.version);
  dev_info(&(pdev->dev), "RECALBOX RGB JAMMA 2 (%d)\n", jamma_config.version);


  jamma_config.unplugged = false;
  return 0;

  err:
    unregister_controllers();
    return -1;
}



static void rrgbj2_remove(struct platform_device *pdev) {
  printk(KERN_INFO "rrgbj2_remove\n");
  unregister_controllers();
}


static const struct of_device_id rrgbj2_match[] = {
  {.compatible = "raspberrypi,recalboxrgbjamma2mcp"},
  {},
};

MODULE_DEVICE_TABLE(of, rrgbj2_match);

static struct platform_driver rrgbj2_driver = {
  .probe  = rrgbj2_probe,
  .remove = rrgbj2_remove,
  .driver = {
    .name        = "recalboxrgbjamma2mcp",
    .of_match_table    = rrgbj2_match,
  },
};

static int power_off(struct sys_off_data *data);

static ssize_t rgbjamma2_plugged_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
  return sysfs_emit(buf, jamma_config.unplugged ? "false" : "true");
}

static struct kobj_attribute rgbjamma2_plugged_attr = __ATTR(rgbjamma2_plugged, 0444, rgbjamma2_plugged_show, NULL);
static struct kobject *jamma2_kobj;

static int __init rrgbj2_init(void) {
  printk(KERN_INFO "recalboxrgbjamma2: init\n");
  if(platform_driver_register(&rrgbj2_driver)){
    printk(KERN_INFO "recalboxrgbjamma2: init failed\n");
    return -ENODEV;
  }

  jamma_config.gpio_chip_2 = NULL;
  jamma_config.expander = NULL;
  jamma_config.hk_on_start = true;
  jamma_config.sound_on_start = true;
  jamma_config.credit_on_start_btn1 = true;
  jamma_config.exit_on_start = true;
  jamma_config.autofire = false;
  jamma_config.autofire_time = 50000000;// 50ms
  jamma_config.i2s = true;
  jamma_config.videofilter = false;
  jamma_config.buttons_on_jamma = 5;
  jamma_config.amp_disable = 0;
  jamma_config.amp_boost = 0;
  jamma_config.player_count = 2;
  jamma_config.dual_joy = false;
  jamma_config.debounce_time_ms = 10;
  jamma_config.coin_counter = true;
  jamma_config.unplugged = true;

  mutex_init(&jamma_config.process_mutex);
//  printk(KERN_INFO "recalboxrgbjamma2: registering controllers\n");
//  mutex_lock(&jamma_config.process_mutex);
//  register_controllers();
//  mutex_unlock(&jamma_config.process_mutex);

  jamma2_kobj = kobject_create_and_add("recalboxrgbjamma2", kernel_kobj);
  if(!jamma2_kobj) {
    printk(KERN_ERR "recalboxrgbjamma2: unable to create /sys/kernel/recalboxrgbjamma2\n");
  } else {
    if(sysfs_create_file(jamma2_kobj, &rgbjamma2_plugged_attr.attr)) {
      printk(KERN_ERR "recalboxrgbjamma2: unable to create /sys/kernel/recalboxrgbjamma2/rgbjamma2_plugged\n");
      kobject_put(jamma2_kobj);
    }
  }
  register_sys_off_handler(SYS_OFF_MODE_POWER_OFF, SYS_OFF_PRIO_DEFAULT, power_off, NULL);

  return 0;
}

static void __exit rrgbj2_exit(void) {
  printk(KERN_INFO
         "recalboxrgbjamma2: exiting\n");
  if(jamma_config.expander) {
    // TODO set fan OFF
    //pca953x_gpio_direction_output(jamma_config.expander, expander[jamma_config.version][EXP_FAN], 0);
  }

  if(jamma2_kobj){
    sysfs_remove_file(jamma2_kobj, &rgbjamma2_plugged_attr.attr);
    kobject_put(jamma2_kobj);
  }

  if (kthread_stop(jamma_config.config_thread)) {
    printk("recalboxrgbjamma2: can't stop config thread");
  }
  if (kthread_stop(jamma_config.debounce_thread)) {
    printk("recalboxrgbjamma2: can't stop debounce thread");
  }
  mutex_lock(&jamma_config.process_mutex);
  unregister_controllers();
  mutex_unlock(&jamma_config.process_mutex);

  platform_driver_unregister(&rrgbj2_driver);
}
static int power_off(struct sys_off_data *data){
  rrgbj2_exit();
  return 0;
}

/* TODO
- [ ] debounce + debounce_time_ms
- [ ] hk_on_start
- [ ] sound_on_start
- [ ] credit_on_start_btn1
- [ ] exit_on_start
- [ ] autofire
- [ ] autofire_time
- [ ] videofilter
- [ ] buttons_on_jamma
- [ ] amp_disable
- [ ] amp_boost
- [ ] player_count
- [ ] dual_joy
- [ ] coin_counter
- [ ] unplugged
 */
module_init(rrgbj2_init);
module_exit(rrgbj2_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RECALBOX RGB JAMMA 2 Driver");
