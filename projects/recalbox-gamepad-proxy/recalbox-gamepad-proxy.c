
/*
 * Recalbox Gamepad Proxy (R.G.P.) - External Source Mapping Edition
 * Copyright (c) 2026 @digitalLumberjack
 *
 * All rights reserved.
 *
 * Requires Linux kernel 6.12+
 *
 * New in v3.12:
 * - External source mappings (keyboard / mouse buttons / gamepad buttons)
 * - Configurable from device tree and runtime sysfs
 * - Module parameter support for runtime mappings
 * - Optional source or target button replacement for external mappings
 * - Optional inverted logic for external button mappings
 * - Generic source-input inversion rules (apply even without external mappings)
 *
 * Runtime mapping format (multiple entries separated by ';'):
 *   <source-name>:<src-type>:<src-code>:<dst-code>:<mode>[:<target-gamepad-name>[:<src-value>]]
 *
 * Example:
 *   PCsensor FootSwitch Keyboard:1:48:305:hold
 *   Arcade Stick:1:304:305:hold-replace:Player 1
 *   PCsensor FootSwitch Keyboard:1:48:305:hold-replace-target:RecalboxJVS Gamepad #1
 *   FootSwitch:1:48:305:hold-invert
 *
 * Meaning:
 *   source-name        = substring matched against input device name
 *   src-type           = input event type (currently EV_KEY only, so '1')
 *   src-code           = input event code from source device (e.g. KEY_B = 48)
 *   dst-code           = key/button code injected on the proxy (e.g. BTN_EAST = 305)
 *   mode               = "hold", "tap", with optional "replace-source", "replace-target" and/or "invert"
 *                        examples: "hold-replace", "hold-replace-target", "tap-invert", "hold-invert-replace-target"
 *   target-gamepad-name= optional substring matched against proxied gamepad name
 *   src-value          = optional exact source value used by tap mode (default: 1)
 *
 * DT external mappings also accept the boolean properties:
 *   replace-source;
 *   replace-target;
 *   invert-logic;
 *
 * Generic input inversion runtime format (multiple entries separated by ';'):
 *   <source-name>:<src-type>:<src-code>
 * Example:
 *   GPIO Gamepad:1:304
 *
 * DT generic input inversion nodes live under:
 *   inverted-inputs { ... };
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/kref.h>
#include <linux/device.h>
#include <linux/bitmap.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/rwlock.h>
#include <linux/mutex.h>
#include <linux/math64.h>

#define DRV_NAME      "recalbox_gamepad_proxy"
#define ACRONYM       "(R.G.P.)"
#define DRV_VERSION   "3.22"

#define VENDOR_VIRT   0xFFFF
#define PRODUCT_VIRT  0x0001

#define RGP_MAX_NAME_LEN            96
#define RGP_MAX_EXTERNAL_MAPS       32
#define RGP_EXTERNAL_MAPS_RAW_LEN   2048
#define RGP_INVERTED_INPUTS_RAW_LEN 2048

/* Pure parsing/match logic, shared with the host unit tests (test/). */
#include "rgp_logic.h"
#define RGP_MAX_FILTERS             8

enum {
  RGP_COMBO_MOD_START    = 0,
  RGP_COMBO_MOD_MODE     = 1,
  RGP_COMBO_MOD_DISABLED = 2,
};

/* ============================================================================
 * MODULE PARAMETERS (can be overridden by DTS)
 * ============================================================================ */

static bool debug = true;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Enable verbose logs");

static bool do_grab = true;
module_param(do_grab, bool, 0644);
MODULE_PARM_DESC(do_grab, "Grab the real device (exclusive).");

static int combo_modifier = RGP_COMBO_MOD_START;
module_param(combo_modifier, int, 0644);
MODULE_PARM_DESC(combo_modifier, "Combo modifier: 0=START, 1=MODE, 2=DISABLED");

static unsigned int start_longpress_ms = 4000;
module_param(start_longpress_ms, uint, 0644);
MODULE_PARM_DESC(start_longpress_ms, "Long press on modifier (ms) to emit START+MODE (0 disables)");

static int stick_threshold = 16000;
module_param(stick_threshold, int, 0644);
MODULE_PARM_DESC(stick_threshold, "Default ABS threshold for stick direction combos");

static int chord_stick_threshold = 24000;
module_param(chord_stick_threshold, int, 0644);
MODULE_PARM_DESC(chord_stick_threshold,
                 "ABS engage threshold while modifier is held (hysteresis: stick_threshold remains the disengage threshold). Must be >= stick_threshold.");

static unsigned int exit_combo_release_ms = 50;
module_param(exit_combo_release_ms, uint, 0644);
MODULE_PARM_DESC(exit_combo_release_ms, "Delay before releasing exit combo keys (ms)");

static bool enable_thumbstick_exit = false;
module_param(enable_thumbstick_exit, bool, 0644);
MODULE_PARM_DESC(enable_thumbstick_exit, "Enable exit combo (START+MODE) when pressing both thumbsticks (L3+R3)");

static unsigned int modifier_tap_delay_ms = 100;
module_param(modifier_tap_delay_ms, uint, 0644);
MODULE_PARM_DESC(modifier_tap_delay_ms, "Delay between press and release when modifier is tapped alone (ms)");

static char device_name_filter[RGP_MAX_NAME_LEN] = "";
module_param_string(device_name, device_name_filter, sizeof(device_name_filter), 0644);
MODULE_PARM_DESC(device_name, "Filter by device name (comma-separated substrings, empty = all gamepads)");

/* Parsed filter lists.
 * dt_filters[]   : populated once at probe from DT "device-filter", never cleared by sysfs.
 * user_filters[] : populated from module param at init and from sysfs writes; freely editable.
 * Match policy   : a device passes if (dt_filter_count == 0 && user_filter_count == 0)
 *                  OR its name contains any of the populated substrings (DT or user).
 */
static char rgp_dt_filters[RGP_MAX_FILTERS][RGP_MAX_NAME_LEN];
static int  rgp_dt_filter_count;
static char rgp_user_filters[RGP_MAX_FILTERS][RGP_MAX_NAME_LEN];
static int  rgp_user_filter_count;
static DEFINE_MUTEX(rgp_filters_mutex);

/* Per-instance UID assigned at gp_connect time and exposed on both the source
 * device (always, for any recognized gamepad) and the proxy device (when
 * proxified). Frontend uses it to pair source ↔ proxy unambiguously.
 *
 * UIDs are cached in a (input_dev * → uid) table so they remain stable across
 * filter rescans (which destroy and re-create ctxs but never the underlying
 * input_dev). Entries are evicted only on real disconnect (unplug), detected
 * by the rgp_in_rescan flag wrapping the unregister/register cycle. */
static atomic_t rgp_next_uid = ATOMIC_INIT(1);

struct rgp_uid_entry {
  struct input_dev *dev;
  unsigned int      uid;
  struct list_head  node;
};
static LIST_HEAD(rgp_uid_table);
static DEFINE_SPINLOCK(rgp_uid_table_lock);
static bool rgp_in_rescan;

static unsigned int rgp_get_or_alloc_uid(struct input_dev *dev)
{
  struct rgp_uid_entry *e, *new_e;
  unsigned int uid;

  spin_lock(&rgp_uid_table_lock);
  list_for_each_entry(e, &rgp_uid_table, node) {
    if (e->dev == dev) {
      uid = e->uid;
      spin_unlock(&rgp_uid_table_lock);
      return uid;
    }
  }
  spin_unlock(&rgp_uid_table_lock);

  new_e = kzalloc(sizeof(*new_e), GFP_KERNEL);
  uid = (unsigned int)atomic_inc_return(&rgp_next_uid);
  if (!new_e)
    return uid; /* alloc failure: tag the pad but skip caching */

  new_e->dev = dev;
  new_e->uid = uid;

  spin_lock(&rgp_uid_table_lock);
  /* Re-check after re-acquire: another concurrent gp_connect on same dev
   * may have inserted while we were unlocked. */
  list_for_each_entry(e, &rgp_uid_table, node) {
    if (e->dev == dev) {
      uid = e->uid;
      spin_unlock(&rgp_uid_table_lock);
      kfree(new_e);
      return uid;
    }
  }
  list_add_tail(&new_e->node, &rgp_uid_table);
  spin_unlock(&rgp_uid_table_lock);
  return uid;
}

static void rgp_remove_uid_entry(struct input_dev *dev)
{
  struct rgp_uid_entry *e, *tmp;

  spin_lock(&rgp_uid_table_lock);
  list_for_each_entry_safe(e, tmp, &rgp_uid_table, node) {
    if (e->dev == dev) {
      list_del(&e->node);
      kfree(e);
      break;
    }
  }
  spin_unlock(&rgp_uid_table_lock);
}

static void rgp_uid_table_destroy(void)
{
  struct rgp_uid_entry *e, *tmp;

  spin_lock(&rgp_uid_table_lock);
  list_for_each_entry_safe(e, tmp, &rgp_uid_table, node) {
    list_del(&e->node);
    kfree(e);
  }
  spin_unlock(&rgp_uid_table_lock);
}

/* Triggered from device_filter_store() to re-scan all input devices against
 * the updated filter list (the actual workfn is defined later, near the
 * input_handler it manipulates). */
static struct work_struct rgp_filter_rescan_work;
static void rgp_filter_rescan_workfn(struct work_struct *w);
static bool main_handler_registered;

static unsigned int debounce_ms = 0;
module_param(debounce_ms, uint, 0644);
MODULE_PARM_DESC(debounce_ms, "Debounce delay for buttons and axes in ms (0 disables, max 500)");

static bool enable_longpress_exit = true;
module_param(enable_longpress_exit, bool, 0644);
MODULE_PARM_DESC(enable_longpress_exit, "Enable SHIFT long press = exit combo (START+MODE)");

static bool enable_shift_mode_passthrough = true;
module_param(enable_shift_mode_passthrough, bool, 0644);
MODULE_PARM_DESC(enable_shift_mode_passthrough, "Enable SHIFT + any button/axis = MODE + button/axis");

static bool enable_shift_volume = true;
module_param(enable_shift_volume, bool, 0644);
MODULE_PARM_DESC(enable_shift_volume, "Enable SHIFT + Up/Down = Volume Up/Down");

static bool enable_shift_credit = true;
module_param(enable_shift_credit, bool, 0644);
MODULE_PARM_DESC(enable_shift_credit, "Enable SHIFT + SOUTH = SELECT (credit)");

static char external_maps_cfg[RGP_EXTERNAL_MAPS_RAW_LEN] = "";
module_param_string(external_maps, external_maps_cfg, sizeof(external_maps_cfg), 0644);
MODULE_PARM_DESC(external_maps,
                 "External runtime mappings: "
                 "<source-name>:<src-type>:<src-code>:<dst-code>:<mode>[:<target-gamepad-name>[:<src-value>]]; "
                 "mode=hold|tap|hold-replace|tap-replace|hold-replace-target|tap-replace-target");

static char inverted_inputs_cfg[RGP_INVERTED_INPUTS_RAW_LEN] = "";
module_param_string(inverted_inputs, inverted_inputs_cfg, sizeof(inverted_inputs_cfg), 0644);
MODULE_PARM_DESC(inverted_inputs,
                 "Generic source input inversions: "
                 "<source-name>:<src-type>:<src-code>[;<source-name>:<src-type>:<src-code>...]");

#define LOGI(fmt, ...) do { if (debug) pr_info(DRV_NAME ": " fmt, ##__VA_ARGS__); } while (0)
#define LOGE(fmt, ...) pr_err(DRV_NAME ": " fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) pr_warn(DRV_NAME ": " fmt, ##__VA_ARGS__)

/* Tap-modifier debug traces — compile-time gated (see jamma2 DEBUG_ENABLED). */
#define TAP_DEBUG 0
#if TAP_DEBUG
  #define TAPDBG(fmt, ...) pr_info(DRV_NAME ": [tapdbg] " fmt, ##__VA_ARGS__)
#else
  #define TAPDBG(fmt, ...) do { } while (0)
#endif

static struct workqueue_struct *gp_wq;

/* ============================================================================
 * EXTERNAL SOURCE MAPPINGS
 * ============================================================================ */

/* struct rgp_external_binding lives in rgp_logic.h (shared with host tests). */

struct rgp_inverted_input {
  char source_name[RGP_MAX_NAME_LEN];
  unsigned int src_type;
  unsigned int src_code;
};

static struct rgp_external_binding rgp_dt_bindings[RGP_MAX_EXTERNAL_MAPS];
static int rgp_dt_binding_count;
static struct rgp_external_binding rgp_runtime_bindings[RGP_MAX_EXTERNAL_MAPS];
static int rgp_runtime_binding_count;
static struct rgp_inverted_input rgp_dt_inverted_inputs[RGP_MAX_EXTERNAL_MAPS];
static int rgp_dt_inverted_input_count;
static struct rgp_inverted_input rgp_runtime_inverted_inputs[RGP_MAX_EXTERNAL_MAPS];
static int rgp_runtime_inverted_input_count;

/* DT-configured analog credit triggers. Each entry maps an axis direction
 * (e.g. ABS_RZ +1 = accelerator pressed on a race cab) into a SHIFT+ABS
 * combo that emits BTN_SELECT, in addition to the static BTN_SOUTH rule. */
#define RGP_MAX_CREDIT_ABS 8
struct rgp_credit_abs { u16 code; s8 dir; s16 threshold; };
static struct rgp_credit_abs rgp_credit_abs_rules[RGP_MAX_CREDIT_ABS];
static u8 rgp_credit_abs_count;

static DEFINE_RWLOCK(rgp_bindings_lock);
static DEFINE_MUTEX(rgp_runtime_bindings_mutex);
static DEFINE_MUTEX(rgp_runtime_inverted_inputs_mutex);

/* Per-game input combos (modifier + trigger -> emit). Parsed into runtime
 * rgp_rule[] entries that feed the same shift-rule engine as the static rules,
 * inheriting modifier/trigger suppression + tap/hold. Runtime-only, no-op by
 * default. rgp_runtime_rules[] itself is declared after struct rgp_rule. */
#define RGP_MAX_COMBOS      8
#define RGP_COMBOS_RAW_LEN  2048
static char rgp_combos_cfg[RGP_COMBOS_RAW_LEN] = "";
static DEFINE_RWLOCK(rgp_combos_lock);
static DEFINE_MUTEX(rgp_combos_mutex);

static int rgp_apply_runtime_bindings_from_string(const char *buf);
static int rgp_apply_runtime_inverted_inputs_from_string(const char *buf);
static int rgp_apply_game_combos_from_string(const char *buf);
static void rgp_refresh_active_vdev_keybits(void);

/* ============================================================================
 * SYSFS CLASS FOR RUNTIME CONFIGURATION (kernel 6.4+ API)
 * ============================================================================ */

#define RGP_SYSFS_BOOL_ATTR(_name, _var) \
static ssize_t _name##_show(const struct class *cls, const struct class_attribute *attr, char *buf) \
{ \
	return sprintf(buf, "%d\n", _var ? 1 : 0); \
} \
static ssize_t _name##_store(const struct class *cls, const struct class_attribute *attr, const char *buf, size_t count) \
{ \
	bool val; \
	if (kstrtobool(buf, &val) < 0) \
		return -EINVAL; \
	_var = val; \
	LOGI("Config changed: " #_name "=%d\n", val ? 1 : 0); \
	return count; \
} \
static struct class_attribute class_attr_##_name = __ATTR_RW(_name)

#define RGP_SYSFS_UINT_ATTR(_name, _var, _min, _max) \
static ssize_t _name##_show(const struct class *cls, const struct class_attribute *attr, char *buf) \
{ \
	return sprintf(buf, "%u\n", _var); \
} \
static ssize_t _name##_store(const struct class *cls, const struct class_attribute *attr, const char *buf, size_t count) \
{ \
	unsigned int val; \
	if (kstrtouint(buf, 10, &val) < 0) \
		return -EINVAL; \
	if (val < _min || val > _max) \
		return -EINVAL; \
	_var = val; \
	LOGI("Config changed: " #_name "=%u\n", val); \
	return count; \
} \
static struct class_attribute class_attr_##_name = __ATTR_RW(_name)

#define RGP_SYSFS_INT_ATTR(_name, _var, _min, _max) \
static ssize_t _name##_show(const struct class *cls, const struct class_attribute *attr, char *buf) \
{ \
	return sprintf(buf, "%d\n", _var); \
} \
static ssize_t _name##_store(const struct class *cls, const struct class_attribute *attr, const char *buf, size_t count) \
{ \
	int val; \
	if (kstrtoint(buf, 10, &val) < 0) \
		return -EINVAL; \
	if (val < _min || val > _max) \
		return -EINVAL; \
	_var = val; \
	LOGI("Config changed: " #_name "=%d\n", val); \
	return count; \
} \
static struct class_attribute class_attr_##_name = __ATTR_RW(_name)

/* Define sysfs attributes */
RGP_SYSFS_BOOL_ATTR(debug, debug);

/* Custom store for do_grab: gp_connect() reads do_grab once at attach,
 * so a sysfs change is silent on already-connected devices until they
 * disconnect. Trigger a filter rescan (which unregisters the handler and
 * re-attaches every device) so the new value is applied live, no module
 * reload or reboot needed. */
static ssize_t do_grab_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", do_grab ? 1 : 0);
}
static ssize_t do_grab_store(const struct class *cls, const struct class_attribute *attr, const char *buf, size_t count)
{
  bool val;
  bool changed;
  if (kstrtobool(buf, &val) < 0)
    return -EINVAL;
  changed = (do_grab != val);
  do_grab = val;
  LOGI("Config changed: do_grab=%d%s\n", val ? 1 : 0, changed ? " (re-scanning devices)" : "");
  if (changed && READ_ONCE(main_handler_registered))
    schedule_work(&rgp_filter_rescan_work);
  return count;
}
static struct class_attribute class_attr_do_grab = __ATTR_RW(do_grab);
RGP_SYSFS_INT_ATTR(combo_modifier, combo_modifier, 0, 2);
RGP_SYSFS_UINT_ATTR(start_longpress_ms, start_longpress_ms, 0, 30000);
RGP_SYSFS_INT_ATTR(stick_threshold, stick_threshold, 0, 32767);
RGP_SYSFS_INT_ATTR(chord_stick_threshold, chord_stick_threshold, 0, 32767);
RGP_SYSFS_UINT_ATTR(exit_combo_release_ms, exit_combo_release_ms, 10, 1000);
RGP_SYSFS_BOOL_ATTR(enable_thumbstick_exit, enable_thumbstick_exit);
RGP_SYSFS_UINT_ATTR(modifier_tap_delay_ms, modifier_tap_delay_ms, 0, 5000);
RGP_SYSFS_UINT_ATTR(debounce_ms, debounce_ms, 0, 500);
RGP_SYSFS_BOOL_ATTR(enable_longpress_exit, enable_longpress_exit);
RGP_SYSFS_BOOL_ATTR(enable_shift_mode_passthrough, enable_shift_mode_passthrough);
RGP_SYSFS_BOOL_ATTR(enable_shift_volume, enable_shift_volume);
RGP_SYSFS_BOOL_ATTR(enable_shift_credit, enable_shift_credit);

/* Version attribute (read-only) */
static ssize_t version_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  return sprintf(buf, "%s\n", DRV_VERSION);
}
static struct class_attribute class_attr_version = __ATTR_RO(version);

/* Parse a comma-separated filter string into an array of NUL-terminated substrings.
 * Whitespace is trimmed, empty entries skipped, overlong entries truncated.
 * Returns the number of populated rows (0..RGP_MAX_FILTERS). */
static int rgp_parse_filter_list(const char *src, char dst[][RGP_MAX_NAME_LEN])
{
  int n = 0;
  const char *p;

  memset(dst, 0, sizeof(char) * RGP_MAX_FILTERS * RGP_MAX_NAME_LEN);

  if (!src)
    return 0;

  p = src;
  while (*p && n < RGP_MAX_FILTERS) {
    const char *comma;
    size_t len;

    while (*p == ' ' || *p == '\t')
      p++;

    comma = strchr(p, ',');
    len = comma ? (size_t)(comma - p) : strlen(p);

    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t'))
      len--;

    if (len > 0) {
      if (len >= RGP_MAX_NAME_LEN)
        len = RGP_MAX_NAME_LEN - 1;
      memcpy(dst[n], p, len);
      dst[n][len] = '\0';
      n++;
    }

    if (!comma)
      break;
    p = comma + 1;
  }

  return n;
}

/* Returns true if `name` matches any active filter (DT or user).
 * If both lists are empty, all devices are accepted (backward-compatible). */
static bool rgp_device_name_matches(const char *name)
{
  bool ok;
  int i;

  mutex_lock(&rgp_filters_mutex);
  if (rgp_dt_filter_count == 0 && rgp_user_filter_count == 0) {
    mutex_unlock(&rgp_filters_mutex);
    return true;
  }
  if (!name) {
    mutex_unlock(&rgp_filters_mutex);
    return false;
  }
  ok = false;
  for (i = 0; i < rgp_dt_filter_count && !ok; i++)
    if (strstr(name, rgp_dt_filters[i]))
      ok = true;
  for (i = 0; i < rgp_user_filter_count && !ok; i++)
    if (strstr(name, rgp_user_filters[i]))
      ok = true;
  mutex_unlock(&rgp_filters_mutex);
  return ok;
}


/* Device name attribute (read/write).
 * Read returns the user filter (raw comma-separated form). DT filter is enforced
 * separately and not exposed here. */
static ssize_t device_filter_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  return sprintf(buf, "%s\n", device_name_filter);
}
static ssize_t device_filter_store(const struct class *cls, const struct class_attribute *attr, const char *buf, size_t count)
{
  char prev[RGP_MAX_NAME_LEN];
  size_t len = count;
  bool changed;
  int i;

  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    len--;

  if (len >= sizeof(device_name_filter))
    return -EINVAL;

  mutex_lock(&rgp_filters_mutex);
  memcpy(prev, device_name_filter, sizeof(prev));
  memset(device_name_filter, 0, sizeof(device_name_filter));
  if (len > 0)
    memcpy(device_name_filter, buf, len);
  rgp_user_filter_count = rgp_parse_filter_list(device_name_filter, rgp_user_filters);
  changed = (memcmp(prev, device_name_filter, sizeof(prev)) != 0);
  mutex_unlock(&rgp_filters_mutex);

  if (!changed)
    return count;

  if (rgp_user_filter_count == 0)
    LOGI("Config changed: device_filter cleared (DT filters still active: %d)\n",
         rgp_dt_filter_count);
  else
    for (i = 0; i < rgp_user_filter_count; i++)
      LOGI("Config changed: device_filter[%d]=\"%s\"\n", i, rgp_user_filters[i]);

  /* Re-evaluate already-connected input devices against the new filter.
   * The work runs asynchronously on the system workqueue. */
  if (READ_ONCE(main_handler_registered))
    schedule_work(&rgp_filter_rescan_work);

  return count;
}
static struct class_attribute class_attr_device_filter = __ATTR_RW(device_filter);

/* External mappings attribute (read/write, runtime layer only) */
static ssize_t external_maps_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  ssize_t len;

  read_lock(&rgp_bindings_lock);
  len = snprintf(buf, PAGE_SIZE, "%s\n", external_maps_cfg);
  read_unlock(&rgp_bindings_lock);

  return len;
}

static ssize_t external_maps_store(const struct class *cls, const struct class_attribute *attr,
                                   const char *buf, size_t count)
{
  char *tmp;
  size_t len = count;
  int ret;

  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    len--;

  if (len >= RGP_EXTERNAL_MAPS_RAW_LEN)
    return -EINVAL;

  tmp = kmalloc(len + 1, GFP_KERNEL);
  if (!tmp)
    return -ENOMEM;

  if (len > 0)
    memcpy(tmp, buf, len);
  tmp[len] = '\0';

  ret = rgp_apply_runtime_bindings_from_string(tmp);
  kfree(tmp);

  if (ret < 0)
    return ret;

  return count;
}
static struct class_attribute class_attr_external_maps = __ATTR_RW(external_maps);

/* Generic inverted inputs attribute (read/write, runtime layer only) */
static ssize_t inverted_inputs_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  ssize_t len;

  read_lock(&rgp_bindings_lock);
  len = snprintf(buf, PAGE_SIZE, "%s\n", inverted_inputs_cfg);
  read_unlock(&rgp_bindings_lock);

  return len;
}

static ssize_t inverted_inputs_store(const struct class *cls, const struct class_attribute *attr,
                                     const char *buf, size_t count)
{
  char *tmp;
  size_t len = count;
  int ret;

  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    len--;

  if (len >= RGP_INVERTED_INPUTS_RAW_LEN)
    return -EINVAL;

  tmp = kmalloc(len + 1, GFP_KERNEL);
  if (!tmp)
    return -ENOMEM;

  if (len > 0)
    memcpy(tmp, buf, len);
  tmp[len] = '\0';

  ret = rgp_apply_runtime_inverted_inputs_from_string(tmp);
  kfree(tmp);

  if (ret < 0)
    return ret;

  return count;
}
static struct class_attribute class_attr_inverted_inputs = __ATTR_RW(inverted_inputs);

/* Per-game combos attribute (read/write, runtime layer only) */
static ssize_t combos_show(const struct class *cls, const struct class_attribute *attr, char *buf)
{
  ssize_t len;

  read_lock(&rgp_combos_lock);
  len = snprintf(buf, PAGE_SIZE, "%s\n", rgp_combos_cfg);
  read_unlock(&rgp_combos_lock);

  return len;
}

static ssize_t combos_store(const struct class *cls, const struct class_attribute *attr,
                            const char *buf, size_t count)
{
  char *tmp;
  size_t len = count;
  int ret;

  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    len--;

  if (len >= RGP_COMBOS_RAW_LEN)
    return -EINVAL;

  tmp = kmalloc(len + 1, GFP_KERNEL);
  if (!tmp)
    return -ENOMEM;

  if (len > 0)
    memcpy(tmp, buf, len);
  tmp[len] = '\0';

  ret = rgp_apply_game_combos_from_string(tmp);
  kfree(tmp);

  if (ret < 0)
    return ret;

  return count;
}
static struct class_attribute class_attr_combos = __ATTR_RW(combos);

/* Array of all class attributes */
static struct class_attribute *rgp_class_attrs[] = {
  &class_attr_version,
  &class_attr_debug,
  &class_attr_do_grab,
  &class_attr_combo_modifier,
  &class_attr_start_longpress_ms,
  &class_attr_stick_threshold,
  &class_attr_chord_stick_threshold,
  &class_attr_exit_combo_release_ms,
  &class_attr_enable_thumbstick_exit,
  &class_attr_modifier_tap_delay_ms,
  &class_attr_debounce_ms,
  &class_attr_enable_longpress_exit,
  &class_attr_enable_shift_mode_passthrough,
  &class_attr_enable_shift_volume,
  &class_attr_enable_shift_credit,
  &class_attr_device_filter,
  &class_attr_external_maps,
  &class_attr_inverted_inputs,
  &class_attr_combos,
  NULL,
};

static struct class *rgp_class;

static int rgp_create_class_files(void)
{
  int i, err;

  for (i = 0; rgp_class_attrs[i] != NULL; i++) {
    err = class_create_file(rgp_class, rgp_class_attrs[i]);
    if (err) {
      LOGE("Failed to create sysfs file %s: %d\n",
           rgp_class_attrs[i]->attr.name, err);
      while (--i >= 0)
        class_remove_file(rgp_class, rgp_class_attrs[i]);
      return err;
    }
  }
  return 0;
}

static void rgp_remove_class_files(void)
{
  int i;

  for (i = 0; rgp_class_attrs[i] != NULL; i++)
    class_remove_file(rgp_class, rgp_class_attrs[i]);
}

/* ============================================================================
 * DEVICE TREE PARSING
 * ============================================================================ */

/* rgp_name_matches() lives in rgp_logic.h (shared with host tests). */

static int rgp_parse_dt_external_mapping(struct device_node *node,
                                         struct rgp_external_binding *b)
{
  const char *str;
  u32 val;

  memset(b, 0, sizeof(*b));
  b->src_value = -1;
  b->dst_tap = false;
  b->replace_source = false;
  b->replace_target = false;
  b->invert_logic = false;

  if (of_property_read_string(node, "source-name", &str))
    return -EINVAL;
  if (strscpy(b->source_name, str, sizeof(b->source_name)) < 0)
    return -EINVAL;

  if (!of_property_read_u32(node, "source-type", &val))
    b->src_type = val;
  else
    b->src_type = EV_KEY;

  if (b->src_type != EV_KEY) {
    LOGW("DT external mapping \"%s\": unsupported source-type %u (only EV_KEY is supported)\n",
         node->name, b->src_type);
    return -EOPNOTSUPP;
  }

  if (of_property_read_u32(node, "source-code", &val))
    return -EINVAL;
  b->src_code = val;

  if (!of_property_read_u32(node, "source-value", &val))
    b->src_value = (int)val;

  if (of_property_read_u32(node, "target-code", &val))
    return -EINVAL;
  b->dst_code = val;

  if (of_property_read_bool(node, "tap"))
    b->dst_tap = true;
  if (of_property_read_bool(node, "replace-source"))
    b->replace_source = true;
  if (of_property_read_bool(node, "replace-target"))
    b->replace_target = true;
  if (of_property_read_bool(node, "invert-logic"))
    b->invert_logic = true;

  if (!of_property_read_string(node, "target-gamepad-name", &str))
    strscpy(b->target_gamepad_name, str, sizeof(b->target_gamepad_name));

  return 0;
}

static int rgp_parse_dt_inverted_input(struct device_node *node,
                                       struct rgp_inverted_input *inv)
{
  const char *str;
  u32 val;

  memset(inv, 0, sizeof(*inv));

  if (of_property_read_string(node, "source-name", &str))
    return -EINVAL;
  if (strscpy(inv->source_name, str, sizeof(inv->source_name)) < 0)
    return -EINVAL;

  if (!of_property_read_u32(node, "source-type", &val))
    inv->src_type = val;
  else
    inv->src_type = EV_KEY;

  if (inv->src_type != EV_KEY) {
    LOGW("DT inverted input \"%s\": unsupported source-type %u (only EV_KEY is supported)\n",
         node->name, inv->src_type);
    return -EOPNOTSUPP;
  }

  if (of_property_read_u32(node, "source-code", &val))
    return -EINVAL;
  inv->src_code = val;

  return 0;
}


static void rgp_parse_dt(struct device_node *np)
{
  u32 val;
  const char *str;
  struct device_node *maps_np, *inv_np, *child;
  struct rgp_external_binding *dt_tmp = NULL;
  struct rgp_inverted_input *inv_tmp = NULL;
  int dt_tmp_count = 0;
  int inv_tmp_count = 0;

  if (!np) {
    LOGI("No device tree node, using module parameters\n");
    return;
  }

  LOGI("Parsing device tree configuration\n");

  dt_tmp = kcalloc(RGP_MAX_EXTERNAL_MAPS, sizeof(*dt_tmp), GFP_KERNEL);
  inv_tmp = kcalloc(RGP_MAX_EXTERNAL_MAPS, sizeof(*inv_tmp), GFP_KERNEL);
  if (!dt_tmp || !inv_tmp) {
    LOGE("Failed to allocate DT parse buffers\n");
    kfree(dt_tmp);
    kfree(inv_tmp);
    return;
  }

  if (of_property_read_bool(np, "debug"))
    debug = true;
  if (of_property_read_bool(np, "no-debug"))
    debug = false;

  if (of_property_read_bool(np, "grab"))
    do_grab = true;
  if (of_property_read_bool(np, "no-grab"))
    do_grab = false;

  if (!of_property_read_u32(np, "combo-modifier", &val)) {
    if (val <= RGP_COMBO_MOD_DISABLED)
      combo_modifier = val;
  }

  if (!of_property_read_u32(np, "longpress-ms", &val))
    start_longpress_ms = val;

  if (!of_property_read_u32(np, "stick-threshold", &val)) {
    if (val <= 32767)
      stick_threshold = val;
  }

  if (!of_property_read_u32(np, "chord-stick-threshold", &val)) {
    if (val <= 32767)
      chord_stick_threshold = val;
  }

  if (!of_property_read_u32(np, "exit-combo-release-ms", &val)) {
    if (val >= 10 && val <= 1000)
      exit_combo_release_ms = val;
  }

  if (of_property_read_bool(np, "thumbstick-exit"))
    enable_thumbstick_exit = true;
  if (of_property_read_bool(np, "no-thumbstick-exit"))
    enable_thumbstick_exit = false;

  if (!of_property_read_u32(np, "modifier-tap-delay-ms", &val)) {
    if (val <= 5000)
      modifier_tap_delay_ms = val;
  }

  if (!of_property_read_u32(np, "debounce-ms", &val)) {
    if (val <= 500)
      debounce_ms = val;
  }

  if (of_property_read_bool(np, "longpress-exit"))
    enable_longpress_exit = true;
  if (of_property_read_bool(np, "no-longpress-exit"))
    enable_longpress_exit = false;

  if (of_property_read_bool(np, "shift-mode-passthrough"))
    enable_shift_mode_passthrough = true;
  if (of_property_read_bool(np, "no-shift-mode-passthrough"))
    enable_shift_mode_passthrough = false;

  if (of_property_read_bool(np, "shift-volume"))
    enable_shift_volume = true;
  if (of_property_read_bool(np, "no-shift-volume"))
    enable_shift_volume = false;

  if (of_property_read_bool(np, "shift-credit"))
    enable_shift_credit = true;
  if (of_property_read_bool(np, "no-shift-credit"))
    enable_shift_credit = false;

  {
    int n = of_property_count_u32_elems(np, "shift-credit-abs");
    if (n > 0) {
      if (n % 3 != 0) {
        LOGW("shift-credit-abs: expected triplets <code dir threshold>, got %d elems\n", n);
      } else {
        u32 buf[RGP_MAX_CREDIT_ABS * 3];
        int max = min(n, (int)(RGP_MAX_CREDIT_ABS * 3));
        if (!of_property_read_u32_array(np, "shift-credit-abs", buf, max)) {
          int i;
          rgp_credit_abs_count = max / 3;
          for (i = 0; i < rgp_credit_abs_count; i++) {
            rgp_credit_abs_rules[i].code      = (u16)buf[i*3];
            rgp_credit_abs_rules[i].dir       = (s8)(s32)buf[i*3 + 1];
            rgp_credit_abs_rules[i].threshold = (s16)buf[i*3 + 2];
            LOGI("DT shift-credit-abs[%d]: code=%u dir=%d threshold=%d\n",
                 i, rgp_credit_abs_rules[i].code,
                 rgp_credit_abs_rules[i].dir, rgp_credit_abs_rules[i].threshold);
          }
        }
      }
    }
  }

  if (!of_property_read_string(np, "device-filter", &str)) {
    int i;
    rgp_dt_filter_count = rgp_parse_filter_list(str, rgp_dt_filters);
    for (i = 0; i < rgp_dt_filter_count; i++)
      LOGI("DT device-filter[%d]=\"%s\"\n", i, rgp_dt_filters[i]);
  }

  maps_np = of_get_child_by_name(np, "external-mappings");
  if (maps_np) {
    for_each_available_child_of_node(maps_np, child) {
      struct rgp_external_binding b;
      int ret;

      if (dt_tmp_count >= RGP_MAX_EXTERNAL_MAPS) {
        LOGW("Too many DT external mappings, ignoring \"%s\"\n", child->name);
        continue;
      }

      ret = rgp_parse_dt_external_mapping(child, &b);
      if (ret < 0) {
        LOGW("Skipping invalid DT external mapping \"%s\" (%d)\n", child->name, ret);
        continue;
      }

      dt_tmp[dt_tmp_count++] = b;
      LOGI("DT external mapping[%d]: src=\"%s\" code=%u -> dst=%u mode=%s%s%s target=\"%s\"\n",
           dt_tmp_count - 1, b.source_name, b.src_code, b.dst_code,
           b.dst_tap ? "tap" : "hold",
           b.replace_source ? "-replace-source" : "",
           b.replace_target ? "-replace-target" : "",
           b.invert_logic ? "-invert" : "",
           b.target_gamepad_name[0] ? b.target_gamepad_name : "<first proxy>");
    }
    of_node_put(maps_np);
  }

  inv_np = of_get_child_by_name(np, "inverted-inputs");
  if (inv_np) {
    for_each_available_child_of_node(inv_np, child) {
      struct rgp_inverted_input inv;
      int ret;

      if (inv_tmp_count >= RGP_MAX_EXTERNAL_MAPS) {
        LOGW("Too many DT inverted inputs, ignoring \"%s\"\n", child->name);
        continue;
      }

      ret = rgp_parse_dt_inverted_input(child, &inv);
      if (ret < 0) {
        LOGW("Skipping invalid DT inverted input \"%s\" (%d)\n", child->name, ret);
        continue;
      }

      inv_tmp[inv_tmp_count++] = inv;
      LOGI("DT inverted input[%d]: src=\"%s\" code=%u\n",
           inv_tmp_count - 1, inv.source_name, inv.src_code);
    }
    of_node_put(inv_np);
  }

  write_lock(&rgp_bindings_lock);
  memset(rgp_dt_bindings, 0, sizeof(rgp_dt_bindings));
  if (dt_tmp_count > 0)
    memcpy(rgp_dt_bindings, dt_tmp, sizeof(dt_tmp[0]) * dt_tmp_count);
  rgp_dt_binding_count = dt_tmp_count;
  memset(rgp_dt_inverted_inputs, 0, sizeof(rgp_dt_inverted_inputs));
  if (inv_tmp_count > 0)
    memcpy(rgp_dt_inverted_inputs, inv_tmp, sizeof(inv_tmp[0]) * inv_tmp_count);
  rgp_dt_inverted_input_count = inv_tmp_count;
  write_unlock(&rgp_bindings_lock);

  rgp_refresh_active_vdev_keybits();

  LOGI("DT config: debug=%d grab=%d combo_modifier=%d\n", debug, do_grab, combo_modifier);
  LOGI("DT config: longpress_exit=%d shift_mode=%d shift_volume=%d shift_credit=%d\n",
       enable_longpress_exit, enable_shift_mode_passthrough, enable_shift_volume, enable_shift_credit);
  LOGI("DT config: %d external mapping(s), %d inverted input(s)\n",
       dt_tmp_count, inv_tmp_count);

  kfree(dt_tmp);
  kfree(inv_tmp);
}

/* ============================================================================
 * COMBO MODE HELPERS
 * ============================================================================ */

static inline int rgp_combo_mode(void)
{
  int m = combo_modifier;
  if (m < RGP_COMBO_MOD_START || m > RGP_COMBO_MOD_DISABLED)
    m = RGP_COMBO_MOD_START;
  return m;
}

static inline bool rgp_combos_enabled(void)
{
  return rgp_combo_mode() != RGP_COMBO_MOD_DISABLED;
}

static inline unsigned int rgp_modifier_key(void)
{
  return (rgp_combo_mode() == RGP_COMBO_MOD_MODE) ? BTN_MODE : BTN_START;
}

#define RGP_MODIFIER_KEY      0xFFFFFFFFu
#define RGP_THRESH_DEFAULT    (-1)
#define RGP_THRESH_HAT_EXACT  (0)

enum rgp_rule_type {
  RGP_RULE_KEY = 0,
  RGP_RULE_ABS_DIR = 1,
};

enum rgp_rule_feature {
  RGP_FEATURE_CREDIT = 0,
  RGP_FEATURE_VOLUME = 1,
  RGP_FEATURE_RUNTIME = 2,  /* per-game combo pushed via the `combos` sysfs */
};

struct rgp_rule {
  enum rgp_rule_type type;
  unsigned int trig_code;
  int trig_dir;
  int trig_threshold;
  unsigned int out_code;
  bool out_tap;
  unsigned int mods[4];
  u8 nmods;
  enum rgp_rule_feature feature;
};

static const struct rgp_rule rgp_rules[] = {
  {
    .type = RGP_RULE_KEY,
    .trig_code = BTN_SOUTH,
    .out_code = BTN_SELECT,
    .out_tap = false,
    .mods = { RGP_MODIFIER_KEY }, .nmods = 1,
    .feature = RGP_FEATURE_CREDIT,
  },
  {
    .type = RGP_RULE_ABS_DIR,
    .trig_code = ABS_Y,
    .trig_dir = -1,
    .trig_threshold = RGP_THRESH_DEFAULT,
    .out_code = KEY_VOLUMEUP,
    .out_tap = true,
    .mods = { RGP_MODIFIER_KEY }, .nmods = 1,
    .feature = RGP_FEATURE_VOLUME,
  },
  {
    .type = RGP_RULE_ABS_DIR,
    .trig_code = ABS_Y,
    .trig_dir = +1,
    .trig_threshold = RGP_THRESH_DEFAULT,
    .out_code = KEY_VOLUMEDOWN,
    .out_tap = true,
    .mods = { RGP_MODIFIER_KEY }, .nmods = 1,
    .feature = RGP_FEATURE_VOLUME,
  },
  {
    .type = RGP_RULE_ABS_DIR,
    .trig_code = ABS_HAT0Y,
    .trig_dir = -1,
    .trig_threshold = RGP_THRESH_HAT_EXACT,
    .out_code = KEY_VOLUMEUP,
    .out_tap = true,
    .mods = { RGP_MODIFIER_KEY }, .nmods = 1,
    .feature = RGP_FEATURE_VOLUME,
  },
  {
    .type = RGP_RULE_ABS_DIR,
    .trig_code = ABS_HAT0Y,
    .trig_dir = +1,
    .trig_threshold = RGP_THRESH_HAT_EXACT,
    .out_code = KEY_VOLUMEDOWN,
    .out_tap = true,
    .mods = { RGP_MODIFIER_KEY }, .nmods = 1,
    .feature = RGP_FEATURE_VOLUME,
  },
};

/* Runtime combo rules, parsed from the `combos` sysfs. Same shape as the static
 * rgp_rules[], so they run through the identical firing/suppression path.
 * Protected by rgp_combos_lock. */
static struct rgp_rule rgp_runtime_rules[RGP_MAX_COMBOS];
static int rgp_runtime_rule_count;

static inline bool rgp_rule_enabled(const struct rgp_rule *r)
{
  switch (r->feature) {
    case RGP_FEATURE_CREDIT:
      return enable_shift_credit;
    case RGP_FEATURE_VOLUME:
      return enable_shift_volume;
    default:
      return true;
  }
}

static bool rgp_abs_has_enabled_rules(unsigned int code)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
    const struct rgp_rule *r = &rgp_rules[i];
    if (r->type == RGP_RULE_ABS_DIR && r->trig_code == code && rgp_rule_enabled(r))
      return true;
  }
  if (enable_shift_credit) {
    for (i = 0; i < rgp_credit_abs_count; i++)
      if (rgp_credit_abs_rules[i].code == code)
        return true;
  }
  return false;
}

static int rgp_abs_get_threshold(unsigned int code, bool *use_hat_exact)
{
  int i, thr = 0;
  *use_hat_exact = false;

  for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
    const struct rgp_rule *r = &rgp_rules[i];
    int t;
    if (r->type != RGP_RULE_ABS_DIR || r->trig_code != code)
      continue;
    t = r->trig_threshold;
    if (t == RGP_THRESH_HAT_EXACT)
      *use_hat_exact = true;
    else if (thr == 0) {
      if (t == RGP_THRESH_DEFAULT) t = stick_threshold;
      thr = t;
    }
  }
  if (enable_shift_credit) {
    for (i = 0; i < rgp_credit_abs_count; i++) {
      if (rgp_credit_abs_rules[i].code != code)
        continue;
      /* Credit-abs threshold is axis-specific (e.g. 128 on a 0..255 pedal),
       * override the static stick_threshold which is sized for centered sticks. */
      thr = rgp_credit_abs_rules[i].threshold;
      break;
    }
  }
  return thr;
}

/* ============================================================================
 * DEBOUNCE STATE TRACKING
 * ============================================================================ */

struct gp_debounce_key {
  u64 last_change_time;
  int stable_value;
  int pending_value;
  bool in_debounce;
};

struct gp_debounce_abs {
  u64 last_change_time;
  int stable_value;
  int pending_value;
  bool in_debounce;
  bool initialized;
};

struct gp_ctx {
  struct input_handle handle;
  spinlock_t lock;
  struct input_dev *vdev;
  struct input_dev *vdev_dead;
  char *vname;
  char *vphys;
  DECLARE_BITMAP(keys_down, KEY_CNT);
  DECLARE_BITMAP(vkeys_down, KEY_CNT);
  DECLARE_BITMAP(raw_forward_down, KEY_CNT);
  u16 synth_hold_count[KEY_CNT];
  DECLARE_BITMAP(remapped_down, KEY_CNT);
  u16 remap_out[KEY_CNT];
  DECLARE_BITMAP(mode_chord_down, KEY_CNT);
  u16 mode_chord_count;
  s8 abs_dir_state[ABS_CNT];
  bool abs_mode_active[ABS_CNT];
  /* out_code a hold-combo is currently holding down for this axis (0 = none),
   * so it can be released when the axis leaves its engaging direction. */
  u16 abs_combo_out[ABS_CNT];
  /* true when abs_combo_out[code] was engaged by a modifier-less combo (a plain
   * remap): it is held/released purely on the axis direction, independent of the
   * modifier (unlike a modifier combo, released when the modifier goes up). */
  bool abs_combo_nomod[ABS_CNT];
  bool mod_pending;
  bool mod_used;
  u64  mod_press_ts_ns;   /* ktime_get_ns() at last modifier press, used to
                           * distinguish a real fast tap (<80ms press) from
                           * a firmware autorepeat artifact (~140ms press). */
  struct delayed_work mod_long_work;
  struct delayed_work exit_combo_release_work;
  bool exit_combo_active;
  bool thumbl_down;
  bool thumbr_down;
  bool thumbstick_exit_fired;
  struct delayed_work mod_tap_press_work;
  struct delayed_work mod_tap_release_work;
  bool mod_tap_pending_press;
  /* Coin tap: holds BTN_SELECT pressed long enough for cores to register
   * a coin (rgp_emit_tap_key alone is too fast for some emulators). */
  struct delayed_work credit_tap_release_work;
  bool grabbed;

  /* Debounce state */
  struct gp_debounce_key key_debounce[KEY_CNT];
  struct gp_debounce_abs abs_debounce[ABS_CNT];
  struct hrtimer debounce_timer;
  spinlock_t debounce_lock;

  bool sysfs_created;
  bool uid_attr_source_created;
  bool passive;
  unsigned int uid;
  atomic_t closing;
  struct kref ref;
  struct device *parent_dev;
  struct work_struct create_work;
  struct work_struct destroy_work;
  struct list_head node;
};

struct rgp_aux_ctx {
  struct input_handle handle;
};

static LIST_HEAD(rgp_ctx_list);
static DEFINE_RWLOCK(rgp_ctx_list_lock);

static ssize_t rgp_proxied_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "1\n");
}
static DEVICE_ATTR(rgp_proxied, 0444, rgp_proxied_show, NULL);

/* Per-instance kernel-generated UID. Exposed on every recognized gamepad
 * (the source) and, when proxified, on the matching proxy device with the
 * same value. Lets the frontend pair source ↔ proxy unambiguously. */
static ssize_t rgp_uid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  struct gp_ctx *ctx;
  ssize_t len = 0;

  read_lock(&rgp_ctx_list_lock);
  list_for_each_entry(ctx, &rgp_ctx_list, node) {
    if ((ctx->handle.dev && &ctx->handle.dev->dev == dev) ||
        (ctx->vdev       && &ctx->vdev->dev       == dev)) {
      len = sprintf(buf, "%u\n", ctx->uid);
      break;
    }
  }
  read_unlock(&rgp_ctx_list_lock);

  if (len == 0)
    len = sprintf(buf, "\n");
  return len;
}
static DEVICE_ATTR(rgp_uid, 0444, rgp_uid_show, NULL);

static void gp_ctx_release(struct kref *ref)
{
  struct gp_ctx *ctx = container_of(ref, struct gp_ctx, ref);
  kfree(ctx);
}

static inline void ctx_get(struct gp_ctx *ctx) { kref_get(&ctx->ref); }
static inline void ctx_put(struct gp_ctx *ctx) { kref_put(&ctx->ref, gp_ctx_release); }

static bool is_our_virtual(const struct input_dev *dev)
{
  return dev->id.bustype == BUS_VIRTUAL &&
         dev->id.vendor  == VENDOR_VIRT &&
         dev->id.product == PRODUCT_VIRT;
}

static bool is_likely_gamepad(struct input_dev *dev)
{
  if (is_our_virtual(dev))
    return false;
  if (!test_bit(EV_ABS, dev->evbit))
    return false;
  if (!(test_bit(ABS_X, dev->absbit) && test_bit(ABS_Y, dev->absbit)))
    return false;
  if (!test_bit(EV_KEY, dev->evbit))
    return false;
  if (!(test_bit(BTN_SOUTH, dev->keybit) || test_bit(BTN_EAST, dev->keybit) ||
        test_bit(BTN_GAMEPAD, dev->keybit) || test_bit(BTN_START, dev->keybit)))
    return false;
  return true;
}

static bool is_external_source_candidate(struct input_dev *dev)
{
  if (is_our_virtual(dev))
    return false;
  if (is_likely_gamepad(dev))
    return false; /* gamepads are already observed through the main handler */
  if (!test_bit(EV_KEY, dev->evbit))
    return false;
  return true;
}

static void default_abs_range(int code, int *min, int *max)
{
  if (code >= ABS_HAT0X && code <= ABS_HAT3Y) {
    *min = -1; *max = 1;
  } else if (code == ABS_Z || code == ABS_RZ || code == ABS_GAS ||
             code == ABS_BRAKE || code == ABS_THROTTLE) {
    *min = 0; *max = 255;
  } else {
    *min = -32768; *max = 32767;
  }
}

static bool validate_rules(void)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
    const struct rgp_rule *r = &rgp_rules[i];
    if (r->out_code >= KEY_CNT) {
      LOGE("Rule %d: invalid out_code %u\n", i, r->out_code);
      return false;
    }
    if (r->type == RGP_RULE_KEY && r->trig_code >= KEY_CNT) {
      LOGE("Rule %d: invalid trig_code %u\n", i, r->trig_code);
      return false;
    } else if (r->type == RGP_RULE_ABS_DIR) {
      if (r->trig_code >= ABS_CNT) {
        LOGE("Rule %d: invalid trig_code %u\n", i, r->trig_code);
        return false;
      }
      if (r->trig_dir != -1 && r->trig_dir != 1) {
        LOGE("Rule %d: invalid trig_dir %d\n", i, r->trig_dir);
        return false;
      }
    }
    if (r->nmods > 4) {
      LOGE("Rule %d: too many mods\n", i);
      return false;
    }
  }
  return true;
}

static bool rgp_mods_down(struct gp_ctx *ctx, const struct rgp_rule *r)
{
  u8 i;
  unsigned int modk = rgp_modifier_key();
  for (i = 0; i < r->nmods; i++) {
    unsigned int k = r->mods[i];
    if (k == RGP_MODIFIER_KEY) k = modk;
    if (k >= KEY_CNT || !test_bit(k, ctx->keys_down))
      return false;
  }
  return true;
}

static bool rgp_is_key_rule_trigger(unsigned int code)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
    if (rgp_rules[i].type == RGP_RULE_KEY &&
        rgp_rules[i].trig_code == code &&
        rgp_rule_enabled(&rgp_rules[i]))
      return true;
  }
  return false;
}

/* ============================================================================
 * VIRTUAL KEY STATE HELPERS
 * ============================================================================ */

static bool rgp_logical_key_should_be_down_locked(struct gp_ctx *ctx, unsigned int code)
{
  return test_bit(code, ctx->raw_forward_down) || ctx->synth_hold_count[code] > 0;
}

static void rgp_emit_tap_key(struct input_dev *vdev, unsigned int code)
{
  if (!vdev || code >= KEY_CNT)
    return;

  /* Press and release MUST go through separate input_sync() so they form
   * two distinct evdev packets — otherwise SDL/EmulationStation polls the
   * joystick state and sees only the final "released" state, missing the
   * press transition entirely. */
  input_report_key(vdev, code, 1);
  input_sync(vdev);
  input_report_key(vdev, code, 0);
  input_sync(vdev);
}

static void rgp_emit_hold_key_sync(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value)
{
  unsigned long flags;
  bool changed = false;
  int out_value = 0;

  if (!ctx || !vdev || code >= KEY_CNT)
    return;

  spin_lock_irqsave(&ctx->lock, flags);

  if (value) {
    if (ctx->synth_hold_count[code] < 65535)
      ctx->synth_hold_count[code]++;
  } else {
    if (ctx->synth_hold_count[code] > 0)
      ctx->synth_hold_count[code]--;
  }

  out_value = rgp_logical_key_should_be_down_locked(ctx, code) ? 1 : 0;
  if (!!test_bit(code, ctx->vkeys_down) != !!out_value) {
    changed = true;
    if (out_value)
      __set_bit(code, ctx->vkeys_down);
    else
      __clear_bit(code, ctx->vkeys_down);
  }

  spin_unlock_irqrestore(&ctx->lock, flags);

  LOGI("emit_hold_key_sync: code=%u value=%d synth_count=%u out_value=%d changed=%d keybit=%d\n",
       code, value, ctx->synth_hold_count[code], out_value, changed,
       test_bit(code, vdev->keybit) ? 1 : 0);

  if (changed && test_bit(code, vdev->keybit)) {
    input_report_key(vdev, code, out_value);
    input_sync(vdev);
    LOGI("emit_hold_key_sync: EMITTED code=%u value=%d\n", code, out_value);
  }
}

static void rgp_emit_two_keys_sync(struct gp_ctx *ctx, struct input_dev *vdev,
                                   unsigned int k1, int v1, unsigned int k2, int v2)
{
  unsigned long flags;
  bool changed1 = false, changed2 = false;
  int out1 = 0, out2 = 0;

  if (!ctx || !vdev || k1 >= KEY_CNT || k2 >= KEY_CNT)
    return;

  spin_lock_irqsave(&ctx->lock, flags);

  if (v1) {
    if (ctx->synth_hold_count[k1] < 65535)
      ctx->synth_hold_count[k1]++;
  } else {
    if (ctx->synth_hold_count[k1] > 0)
      ctx->synth_hold_count[k1]--;
  }

  if (v2) {
    if (ctx->synth_hold_count[k2] < 65535)
      ctx->synth_hold_count[k2]++;
  } else {
    if (ctx->synth_hold_count[k2] > 0)
      ctx->synth_hold_count[k2]--;
  }

  out1 = rgp_logical_key_should_be_down_locked(ctx, k1) ? 1 : 0;
  out2 = rgp_logical_key_should_be_down_locked(ctx, k2) ? 1 : 0;

  if (!!test_bit(k1, ctx->vkeys_down) != !!out1) {
    changed1 = true;
    if (out1)
      __set_bit(k1, ctx->vkeys_down);
    else
      __clear_bit(k1, ctx->vkeys_down);
  }

  if (!!test_bit(k2, ctx->vkeys_down) != !!out2) {
    changed2 = true;
    if (out2)
      __set_bit(k2, ctx->vkeys_down);
    else
      __clear_bit(k2, ctx->vkeys_down);
  }

  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!changed1 && !changed2)
    return;

  if (changed1 && test_bit(k1, vdev->keybit))
    input_report_key(vdev, k1, out1);
  if (changed2 && test_bit(k2, vdev->keybit))
    input_report_key(vdev, k2, out2);
  input_sync(vdev);
}

static void rgp_forward_raw_key_sync(struct gp_ctx *ctx, struct input_dev *vdev,
                                     unsigned int code, int value)
{
  unsigned long flags;
  bool changed = false;
  int out_value = 0;

  if (!ctx || !vdev || code >= KEY_CNT)
    return;

  spin_lock_irqsave(&ctx->lock, flags);

  if (value)
    __set_bit(code, ctx->raw_forward_down);
  else
    __clear_bit(code, ctx->raw_forward_down);

  out_value = rgp_logical_key_should_be_down_locked(ctx, code) ? 1 : 0;
  if (!!test_bit(code, ctx->vkeys_down) != !!out_value) {
    changed = true;
    if (out_value)
      __set_bit(code, ctx->vkeys_down);
    else
      __clear_bit(code, ctx->vkeys_down);
  }

  spin_unlock_irqrestore(&ctx->lock, flags);

  if (changed && test_bit(code, vdev->keybit)) {
    input_report_key(vdev, code, out_value);
    input_sync(vdev);
  }
}

static void rgp_emit_exit_combo(struct gp_ctx *ctx, struct input_dev *vdev)
{
  if (!ctx || !vdev)
    return;
  LOGI("Emitting exit combo (START+MODE)\n");
  rgp_emit_two_keys_sync(ctx, vdev, BTN_START, 1, BTN_MODE, 1);
  queue_delayed_work(gp_wq, &ctx->exit_combo_release_work,
                     msecs_to_jiffies(exit_combo_release_ms));
}

/* ============================================================================
 * EXTERNAL BINDING HELPERS
 * ============================================================================ */

static void rgp_collect_external_target_keybits(unsigned long *bitmap)
{
  int i;

  bitmap_zero(bitmap, KEY_CNT);

  read_lock(&rgp_bindings_lock);
  for (i = 0; i < rgp_dt_binding_count; i++) {
    if (rgp_dt_bindings[i].dst_code < KEY_CNT)
      __set_bit(rgp_dt_bindings[i].dst_code, bitmap);
  }
  for (i = 0; i < rgp_runtime_binding_count; i++) {
    if (rgp_runtime_bindings[i].dst_code < KEY_CNT)
      __set_bit(rgp_runtime_bindings[i].dst_code, bitmap);
  }
  read_unlock(&rgp_bindings_lock);

  read_lock(&rgp_combos_lock);
  for (i = 0; i < rgp_runtime_rule_count; i++) {
    if (rgp_runtime_rules[i].out_code < KEY_CNT)
      __set_bit(rgp_runtime_rules[i].out_code, bitmap);
  }
  read_unlock(&rgp_combos_lock);
}

static void rgp_refresh_active_vdev_keybits(void)
{
  struct gp_ctx *ctx;
  DECLARE_BITMAP(extra_keys, KEY_CNT);
  unsigned long flags;

  rgp_collect_external_target_keybits(extra_keys);

  read_lock(&rgp_ctx_list_lock);
  list_for_each_entry(ctx, &rgp_ctx_list, node) {
    struct input_dev *vdev;

    spin_lock_irqsave(&ctx->lock, flags);
    vdev = ctx->vdev;
    spin_unlock_irqrestore(&ctx->lock, flags);

    if (!vdev)
      continue;

    bitmap_or(vdev->keybit, vdev->keybit, extra_keys, KEY_CNT);
  }
  read_unlock(&rgp_ctx_list_lock);
}

/*
 * rgp_parse_binding_mode() and rgp_parse_external_binding_string() live in
 * rgp_logic.h (shared with the host unit tests in test/).
 */

static int rgp_apply_runtime_bindings_from_string(const char *buf)
{
  struct rgp_external_binding *tmp = NULL;
  int tmp_count = 0;
  char *dup = NULL, *entry, *cursor;
  int ret = 0;

  mutex_lock(&rgp_runtime_bindings_mutex);

  if (!buf || buf[0] == '\0') {
    write_lock(&rgp_bindings_lock);
    memset(rgp_runtime_bindings, 0, sizeof(rgp_runtime_bindings));
    rgp_runtime_binding_count = 0;
    external_maps_cfg[0] = '\0';
    write_unlock(&rgp_bindings_lock);

    rgp_refresh_active_vdev_keybits();
    LOGI("Runtime external mappings cleared\n");
    goto out_unlock;
  }

  tmp = kcalloc(RGP_MAX_EXTERNAL_MAPS, sizeof(*tmp), GFP_KERNEL);
  if (!tmp) {
    ret = -ENOMEM;
    goto out_unlock;
  }

  dup = kstrdup(buf, GFP_KERNEL);
  if (!dup) {
    ret = -ENOMEM;
    goto out_free;
  }

  cursor = dup;
  while ((entry = strsep(&cursor, ";")) != NULL) {
    struct rgp_external_binding b;

    entry = strim(entry);
    if (!entry[0])
      continue;

    if (tmp_count >= RGP_MAX_EXTERNAL_MAPS) {
      ret = -E2BIG;
      goto out_free;
    }

    ret = rgp_parse_external_binding_string(entry, &b);
    if (ret < 0)
      goto out_free;

    tmp[tmp_count++] = b;
  }

  write_lock(&rgp_bindings_lock);
  memset(rgp_runtime_bindings, 0, sizeof(rgp_runtime_bindings));
  if (tmp_count > 0)
    memcpy(rgp_runtime_bindings, tmp, sizeof(tmp[0]) * tmp_count);
  rgp_runtime_binding_count = tmp_count;
  strscpy(external_maps_cfg, buf, sizeof(external_maps_cfg));
  write_unlock(&rgp_bindings_lock);

  rgp_refresh_active_vdev_keybits();

  LOGI("Applied %d runtime external mapping(s)\n", tmp_count);
  ret = 0;

out_free:
  kfree(dup);
  kfree(tmp);
out_unlock:
  mutex_unlock(&rgp_runtime_bindings_mutex);
  return ret;
}

/* ============================================================================
 * GAME COMBOS RUNTIME (see rgp_logic.h for the pure parse/match logic)
 * ==========================================================================*/

/* Convert one parsed combo into a runtime rgp_rule so it runs through the same
 * firing/suppression path as the static shift rules. */
static void rgp_game_combo_to_rule(const struct rgp_game_combo *c, struct rgp_rule *r)
{
  memset(r, 0, sizeof(*r));
  r->out_code = c->emit_code;
  r->out_tap = c->emit_tap;
  r->feature = RGP_FEATURE_RUNTIME;

  if (c->mod_code) {
    r->mods[0] = c->mod_code;
    r->nmods = 1;
  } else {
    r->nmods = 0;   /* no modifier = a plain remap, still with suppression */
  }

  if (c->trig_type == EV_ABS) {
    r->type = RGP_RULE_ABS_DIR;
    r->trig_code = c->trig_code;
    r->trig_dir = (c->trig_val > 0) ? 1 : (c->trig_val < 0 ? -1 : 0);
    /* dpad/hat axes report exact +/-1; sticks fall back to stick_threshold. */
    if (c->trig_code >= ABS_HAT0X && c->trig_code <= ABS_HAT3Y)
      r->trig_threshold = RGP_THRESH_HAT_EXACT;
    else
      r->trig_threshold = RGP_THRESH_DEFAULT;
  } else {
    r->type = RGP_RULE_KEY;
    r->trig_code = c->trig_code;
  }
}

static int rgp_apply_game_combos_from_string(const char *buf)
{
  struct rgp_rule tmp[RGP_MAX_COMBOS];
  int tmp_count = 0;
  char *dup = NULL, *entry, *cursor;
  int ret = 0;

  mutex_lock(&rgp_combos_mutex);

  if (!buf || buf[0] == '\0') {
    write_lock(&rgp_combos_lock);
    memset(rgp_runtime_rules, 0, sizeof(rgp_runtime_rules));
    rgp_runtime_rule_count = 0;
    rgp_combos_cfg[0] = '\0';
    write_unlock(&rgp_combos_lock);
    rgp_refresh_active_vdev_keybits();
    LOGI("Game combos cleared\n");
    goto out_unlock;
  }

  dup = kstrdup(buf, GFP_KERNEL);
  if (!dup) {
    ret = -ENOMEM;
    goto out_unlock;
  }

  cursor = dup;
  while ((entry = strsep(&cursor, ";")) != NULL) {
    struct rgp_game_combo c;

    entry = strim(entry);
    if (!entry[0])
      continue;

    if (tmp_count >= RGP_MAX_COMBOS) {
      ret = -E2BIG;
      goto out_free;
    }

    ret = rgp_parse_game_combo_string(entry, &c);
    if (ret < 0)
      goto out_free;

    rgp_game_combo_to_rule(&c, &tmp[tmp_count++]);
  }

  write_lock(&rgp_combos_lock);
  memset(rgp_runtime_rules, 0, sizeof(rgp_runtime_rules));
  if (tmp_count > 0)
    memcpy(rgp_runtime_rules, tmp, sizeof(tmp[0]) * tmp_count);
  rgp_runtime_rule_count = tmp_count;
  strscpy(rgp_combos_cfg, buf, sizeof(rgp_combos_cfg));
  write_unlock(&rgp_combos_lock);

  rgp_refresh_active_vdev_keybits();
  LOGI("Applied %d game combo(s)\n", tmp_count);
  ret = 0;

out_free:
  kfree(dup);
out_unlock:
  mutex_unlock(&rgp_combos_mutex);
  return ret;
}

/* Snapshot the runtime rules into a caller buffer (avoids holding the combos
 * lock inside the ctx lock in the hot path). Returns the count copied. */
static int rgp_snapshot_runtime_rules(struct rgp_rule *dst)
{
  int n;

  read_lock(&rgp_combos_lock);
  n = rgp_runtime_rule_count;
  if (n > 0)
    memcpy(dst, rgp_runtime_rules, sizeof(dst[0]) * n);
  read_unlock(&rgp_combos_lock);

  return n;
}

static int rgp_parse_inverted_input_string(char *entry, struct rgp_inverted_input *inv)
{
  char *cursor = entry;
  char *tok;
  char *fields[3];
  int nfields = 0;
  unsigned int uval;

  memset(inv, 0, sizeof(*inv));

  while ((tok = strsep(&cursor, ":")) != NULL && nfields < ARRAY_SIZE(fields))
    fields[nfields++] = tok;

  if (nfields < 3)
    return -EINVAL;

  if (fields[0][0] == '\0')
    return -EINVAL;

  if (strscpy(inv->source_name, fields[0], sizeof(inv->source_name)) < 0)
    return -EINVAL;

  if (kstrtouint(fields[1], 0, &uval) < 0)
    return -EINVAL;
  inv->src_type = uval;

  if (inv->src_type != EV_KEY)
    return -EOPNOTSUPP;

  if (kstrtouint(fields[2], 0, &uval) < 0)
    return -EINVAL;
  inv->src_code = uval;

  return 0;
}

static int rgp_apply_runtime_inverted_inputs_from_string(const char *buf)
{
  struct rgp_inverted_input *tmp = NULL;
  int tmp_count = 0;
  char *dup = NULL, *entry, *cursor;
  int ret = 0;

  mutex_lock(&rgp_runtime_inverted_inputs_mutex);

  if (!buf || buf[0] == '\0') {
    write_lock(&rgp_bindings_lock);
    memset(rgp_runtime_inverted_inputs, 0, sizeof(rgp_runtime_inverted_inputs));
    rgp_runtime_inverted_input_count = 0;
    inverted_inputs_cfg[0] = '\0';
    write_unlock(&rgp_bindings_lock);

    LOGI("Runtime inverted inputs cleared\n");
    goto out_unlock;
  }

  tmp = kcalloc(RGP_MAX_EXTERNAL_MAPS, sizeof(*tmp), GFP_KERNEL);
  if (!tmp) {
    ret = -ENOMEM;
    goto out_unlock;
  }

  dup = kstrdup(buf, GFP_KERNEL);
  if (!dup) {
    ret = -ENOMEM;
    goto out_free;
  }

  cursor = dup;
  while ((entry = strsep(&cursor, ";")) != NULL) {
    struct rgp_inverted_input inv;

    entry = strim(entry);
    if (!entry[0])
      continue;

    if (tmp_count >= RGP_MAX_EXTERNAL_MAPS) {
      ret = -E2BIG;
      goto out_free;
    }

    ret = rgp_parse_inverted_input_string(entry, &inv);
    if (ret < 0)
      goto out_free;

    tmp[tmp_count++] = inv;
  }

  write_lock(&rgp_bindings_lock);
  memset(rgp_runtime_inverted_inputs, 0, sizeof(rgp_runtime_inverted_inputs));
  if (tmp_count > 0)
    memcpy(rgp_runtime_inverted_inputs, tmp, sizeof(tmp[0]) * tmp_count);
  rgp_runtime_inverted_input_count = tmp_count;
  strscpy(inverted_inputs_cfg, buf, sizeof(inverted_inputs_cfg));
  write_unlock(&rgp_bindings_lock);

  LOGI("Applied %d runtime inverted input(s)\n", tmp_count);
  ret = 0;

out_free:
  kfree(dup);
  kfree(tmp);
out_unlock:
  mutex_unlock(&rgp_runtime_inverted_inputs_mutex);
  return ret;
}

static bool rgp_binding_matches_source(const struct rgp_external_binding *b,
                                       const struct input_dev *src_dev)
{
  return rgp_name_matches(src_dev ? src_dev->name : NULL, b->source_name);
}

static bool rgp_inverted_input_matches_source(const struct rgp_inverted_input *inv,
                                              const struct input_dev *src_dev)
{
  return rgp_name_matches(src_dev ? src_dev->name : NULL, inv->source_name);
}

static bool rgp_is_input_inverted(const struct input_dev *src_dev,
                                  unsigned int type, unsigned int code)
{
  int i;
  bool found = false;

  if (!src_dev || !src_dev->name)
    return false;
  if (type != EV_KEY)
    return false;

  read_lock(&rgp_bindings_lock);
  for (i = 0; i < rgp_dt_inverted_input_count; i++) {
    const struct rgp_inverted_input *r = &rgp_dt_inverted_inputs[i];
    if (r->src_type != type || r->src_code != code)
      continue;
    if (!rgp_inverted_input_matches_source(r, src_dev))
      continue;
    found = true;
    goto out;
  }
  for (i = 0; i < rgp_runtime_inverted_input_count; i++) {
    const struct rgp_inverted_input *r = &rgp_runtime_inverted_inputs[i];
    if (r->src_type != type || r->src_code != code)
      continue;
    if (!rgp_inverted_input_matches_source(r, src_dev))
      continue;
    found = true;
    goto out;
  }
out:
  read_unlock(&rgp_bindings_lock);
  return found;
}

static int rgp_apply_source_input_transform(const struct input_dev *src_dev,
                                            unsigned int type, unsigned int code, int value)
{
  if (type == EV_KEY && (value == 0 || value == 1) &&
      rgp_is_input_inverted(src_dev, type, code)) {
    int new_value = !value;
    LOGI("Generic input inversion on \"%s\": code=%u value=%d -> %d\n",
         src_dev && src_dev->name ? src_dev->name : "(null)",
         code, value, new_value);
    return new_value;
  }

  return value;
}


/* rgp_binding_matches_event() lives in rgp_logic.h (shared with host tests). */

static struct gp_ctx *rgp_find_target_ctx_for_binding(const struct rgp_external_binding *b)
{
  struct gp_ctx *ctx, *ret = NULL;
  unsigned long flags;
  bool match;

  read_lock(&rgp_ctx_list_lock);
  list_for_each_entry(ctx, &rgp_ctx_list, node) {
    if (atomic_read(&ctx->closing))
      continue;

    match = true;
    if (b->target_gamepad_name[0] != '\0')
      match = rgp_name_matches(ctx->handle.dev ? ctx->handle.dev->name : NULL,
                               b->target_gamepad_name);
    if (!match)
      continue;

    spin_lock_irqsave(&ctx->lock, flags);
    if (ctx->vdev) {
      ctx_get(ctx);
      ret = ctx;
      spin_unlock_irqrestore(&ctx->lock, flags);
      break;
    }
    spin_unlock_irqrestore(&ctx->lock, flags);
  }
  read_unlock(&rgp_ctx_list_lock);

  return ret;
}

static bool rgp_ctx_is_binding_target(struct gp_ctx *target_ctx,
                                      const struct rgp_external_binding *b)
{
  struct gp_ctx *ctx;
  unsigned long flags;
  bool is_target = false;
  bool match;

  if (!target_ctx || !b)
    return false;

  read_lock(&rgp_ctx_list_lock);
  list_for_each_entry(ctx, &rgp_ctx_list, node) {
    if (atomic_read(&ctx->closing))
      continue;

    match = true;
    if (b->target_gamepad_name[0] != '\0')
      match = rgp_name_matches(ctx->handle.dev ? ctx->handle.dev->name : NULL,
                               b->target_gamepad_name);
    if (!match)
      continue;

    spin_lock_irqsave(&ctx->lock, flags);
    if (ctx->vdev)
      is_target = (ctx == target_ctx);
    spin_unlock_irqrestore(&ctx->lock, flags);

    if (is_target)
      break;

    if (b->target_gamepad_name[0] == '\0')
      break;
  }
  read_unlock(&rgp_ctx_list_lock);

  return is_target;
}

static bool rgp_should_replace_target_event(struct gp_ctx *target_ctx,
                                            unsigned int type, unsigned int code, int value)
{
  int i;
  bool found = false;

  if (!target_ctx)
    return false;
  if (type != EV_KEY)
    return false;
  if (value != 0 && value != 1)
    return false;

  read_lock(&rgp_bindings_lock);
  for (i = 0; i < rgp_dt_binding_count; i++) {
    const struct rgp_external_binding *b = &rgp_dt_bindings[i];
    if (!b->replace_target || b->dst_code != code)
      continue;
    if (!rgp_ctx_is_binding_target(target_ctx, b))
      continue;
    found = true;
    goto out;
  }
  for (i = 0; i < rgp_runtime_binding_count; i++) {
    const struct rgp_external_binding *b = &rgp_runtime_bindings[i];
    if (!b->replace_target || b->dst_code != code)
      continue;
    if (!rgp_ctx_is_binding_target(target_ctx, b))
      continue;
    found = true;
    goto out;
  }
out:
  read_unlock(&rgp_bindings_lock);
  return found;
}

static bool rgp_should_replace_source_event(struct input_dev *src_dev,
                                           unsigned int type, unsigned int code, int value)
{
  int i;
  bool found = false;

  if (!src_dev || !src_dev->name)
    return false;
  if (type != EV_KEY)
    return false;
  if (value != 0 && value != 1)
    return false;

  read_lock(&rgp_bindings_lock);
  for (i = 0; i < rgp_dt_binding_count; i++) {
    const struct rgp_external_binding *b = &rgp_dt_bindings[i];
    if (!b->replace_source)
      continue;
    if (b->src_type != type || b->src_code != code)
      continue;
    if (!rgp_binding_matches_source(b, src_dev))
      continue;
    found = true;
    goto out;
  }
  for (i = 0; i < rgp_runtime_binding_count; i++) {
    const struct rgp_external_binding *b = &rgp_runtime_bindings[i];
    if (!b->replace_source)
      continue;
    if (b->src_type != type || b->src_code != code)
      continue;
    if (!rgp_binding_matches_source(b, src_dev))
      continue;
    found = true;
    goto out;
  }
out:
  read_unlock(&rgp_bindings_lock);
  return found;
}

static void rgp_process_one_binding(struct input_dev *src_dev,
                                    const struct rgp_external_binding *b,
                                    unsigned int type, unsigned int code, int value)
{
  struct gp_ctx *ctx;
  struct input_dev *vdev;
  unsigned long flags;

  if (!rgp_binding_matches_source(b, src_dev))
    return;
  if (!rgp_binding_matches_event(b, type, code, value))
    return;

  LOGI("process_external_bindings: MATCHED src=\"%s\" code=%u, finding target ctx\n",
       b->source_name, b->src_code);

  ctx = rgp_find_target_ctx_for_binding(b);
  if (!ctx)
    return;

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!vdev) {
    ctx_put(ctx);
    return;
  }

  if (b->dst_code >= KEY_CNT || !test_bit(b->dst_code, vdev->keybit)) {
    LOGW("External mapping dst_code=%u cannot be emitted on proxy \"%s\"\n",
         b->dst_code, vdev->name ? vdev->name : "(null)");
    ctx_put(ctx);
    return;
  }

  {
    int mapped_value = value;

    if (b->invert_logic)
      mapped_value = !value;

    LOGI("External source \"%s\": type=%u code=%u value=%d -> mapped=%d dst=%u on \"%s\"%s%s%s\n",
         src_dev->name, type, code, value, mapped_value, b->dst_code,
         ctx->handle.dev && ctx->handle.dev->name ? ctx->handle.dev->name : "(null)",
         b->replace_source ? " (replace-source)" : "",
         b->replace_target ? " (replace-target)" : "",
         b->invert_logic ? " (invert-logic)" : "");

    if (b->dst_tap)
      rgp_emit_tap_key(vdev, b->dst_code);
    else
      rgp_emit_hold_key_sync(ctx, vdev, b->dst_code, mapped_value ? 1 : 0);
  }

  ctx_put(ctx);
}

static void rgp_process_external_bindings(struct input_dev *src_dev,
                                          unsigned int type, unsigned int code, int value)
{
  struct rgp_external_binding snap;
  int i, dt_n, rt_n;

  if (!src_dev || !src_dev->name)
    return;
  if (type != EV_KEY)
    return;

  LOGI("process_external_bindings: src=\"%s\" type=%u code=%u value=%d\n",
       src_dev->name, type, code, value);

  read_lock(&rgp_bindings_lock);
  dt_n = rgp_dt_binding_count;
  rt_n = rgp_runtime_binding_count;
  read_unlock(&rgp_bindings_lock);

  for (i = 0; i < dt_n; i++) {
    read_lock(&rgp_bindings_lock);
    if (i >= rgp_dt_binding_count) {
      read_unlock(&rgp_bindings_lock);
      break;
    }
    snap = rgp_dt_bindings[i];
    read_unlock(&rgp_bindings_lock);
    rgp_process_one_binding(src_dev, &snap, type, code, value);
  }
  for (i = 0; i < rt_n; i++) {
    read_lock(&rgp_bindings_lock);
    if (i >= rgp_runtime_binding_count) {
      read_unlock(&rgp_bindings_lock);
      break;
    }
    snap = rgp_runtime_bindings[i];
    read_unlock(&rgp_bindings_lock);
    rgp_process_one_binding(src_dev, &snap, type, code, value);
  }
}

static void rgp_exit_combo_release_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(to_delayed_work(work), struct gp_ctx, exit_combo_release_work);
  struct input_dev *vdev;
  unsigned long flags;

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  ctx->exit_combo_active = false;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!vdev)
    return;

  rgp_emit_two_keys_sync(ctx, vdev, BTN_MODE, 0, BTN_START, 0);
}

static void rgp_mod_long_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(to_delayed_work(work), struct gp_ctx, mod_long_work);
  struct input_dev *vdev;
  unsigned long flags;
  bool can_fire;
  unsigned int modk = rgp_modifier_key();

  spin_lock_irqsave(&ctx->lock, flags);
  can_fire = rgp_combos_enabled() &&
             enable_longpress_exit &&
             start_longpress_ms > 0 &&
             !atomic_read(&ctx->closing) &&
             ctx->mod_pending && !ctx->mod_used && (modk < KEY_CNT) &&
             test_bit(modk, ctx->keys_down) && !ctx->exit_combo_active;
  if (can_fire) {
    ctx->mod_used = true;
    ctx->exit_combo_active = true;
    TAPDBG("mod_used=1 cause=longpress_exit\n");
  }
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!can_fire || !vdev)
    return;
  LOGI("Modifier long-press -> emit START+MODE\n");
  rgp_emit_exit_combo(ctx, vdev);
}

static void rgp_mod_tap_press_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(to_delayed_work(work), struct gp_ctx, mod_tap_press_work);
  struct input_dev *vdev;
  unsigned long flags;
  unsigned int modk = rgp_modifier_key();
  bool should_emit = false;

  spin_lock_irqsave(&ctx->lock, flags);
  if (ctx->mod_tap_pending_press && !atomic_read(&ctx->closing)) {
    should_emit = true;
    ctx->mod_tap_pending_press = false;
  }
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (should_emit && vdev && modk < KEY_CNT) {
    TAPDBG("tap PRESS emitted on vdev (code=%u)\n", modk);
    /* Direct emit (do NOT go through rgp_emit_hold_key_sync): a stuck
     * synth_hold_count from an unbalanced exit combo would otherwise
     * keep the logical state at 1 and silently drop the press. */
    if (test_bit(modk, vdev->keybit)) {
      input_report_key(vdev, modk, 1);
      input_sync(vdev);
    }
    queue_delayed_work(gp_wq, &ctx->mod_tap_release_work, msecs_to_jiffies(modifier_tap_delay_ms));
  } else {
    TAPDBG("tap PRESS skipped (should_emit=%d vdev=%p modk=%u)\n",
            should_emit, vdev, modk);
  }
}

static void rgp_mod_tap_release_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(to_delayed_work(work), struct gp_ctx, mod_tap_release_work);
  struct input_dev *vdev;
  unsigned long flags;
  unsigned int modk = rgp_modifier_key();

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!atomic_read(&ctx->closing) && vdev && modk < KEY_CNT) {
    TAPDBG("tap RELEASE emitted on vdev (code=%u, after %ums)\n",
            modk, modifier_tap_delay_ms);
    /* Direct emit, paired with the press above. */
    if (test_bit(modk, vdev->keybit)) {
      input_report_key(vdev, modk, 0);
      input_sync(vdev);
    }
  } else {
    TAPDBG("tap RELEASE skipped (closing=%d vdev=%p modk=%u)\n",
            atomic_read(&ctx->closing), vdev, modk);
  }
}

#define RGP_CREDIT_TAP_HOLD_MS 100

static void rgp_credit_tap_release_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(to_delayed_work(work), struct gp_ctx, credit_tap_release_work);
  struct input_dev *vdev;
  unsigned long flags;

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!atomic_read(&ctx->closing) && vdev && test_bit(BTN_SELECT, vdev->keybit)) {
    input_report_key(vdev, BTN_SELECT, 0);
    input_sync(vdev);
  }
}

/* ============================================================================
 * DEBOUNCE TIMER AND PROCESSING
 * ============================================================================ */

static void rgp_process_key_event_internal(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value);
static void rgp_process_abs_event_internal(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value);

/*
 * Timer function: called after debounce period to emit pending changes.
 *
 * FIX v3.7: ABS return-to-neutral (pending=0) is now emitted instead of
 * being silently dropped. The previous logic assumed a quick return to 0
 * was bouncing, but on arcade joysticks with micro-switches, short presses
 * followed by a legitimate release are normal. Dropping the release caused
 * stuck directions.
 */
static enum hrtimer_restart rgp_debounce_timer_fn(struct hrtimer *timer)
{
  struct gp_ctx *ctx = container_of(timer, struct gp_ctx, debounce_timer);
  struct input_dev *vdev;
  unsigned long flags;
  u64 now_ns = ktime_get_ns();
  u64 debounce_ns = (u64)debounce_ms * 1000000ULL;
  u64 next_expire_ns = 0;
  bool need_restart = false;
  int i;

  if (atomic_read(&ctx->closing))
    return HRTIMER_NORESTART;

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!vdev)
    return HRTIMER_NORESTART;

  spin_lock_irqsave(&ctx->debounce_lock, flags);

  /* Process key debounces */
  for (i = 0; i < KEY_CNT; i++) {
    struct gp_debounce_key *dk = &ctx->key_debounce[i];
    u64 elapsed;

    if (!dk->in_debounce)
      continue;

    elapsed = now_ns - dk->last_change_time;
    if (elapsed >= debounce_ns) {
      dk->in_debounce = false;
      if (dk->pending_value != dk->stable_value) {
        int val = dk->pending_value;
        dk->stable_value = val;
        spin_unlock_irqrestore(&ctx->debounce_lock, flags);
        rgp_process_key_event_internal(ctx, vdev, i, val);
        spin_lock_irqsave(&ctx->debounce_lock, flags);
      }
    } else {
      u64 remaining = debounce_ns - elapsed;
      if (!need_restart || remaining < next_expire_ns) {
        next_expire_ns = remaining;
        need_restart = true;
      }
    }
  }

  /* Process abs debounces */
  for (i = 0; i < ABS_CNT; i++) {
    struct gp_debounce_abs *da = &ctx->abs_debounce[i];
    u64 elapsed;

    if (!da->in_debounce)
      continue;

    elapsed = now_ns - da->last_change_time;
    if (elapsed >= debounce_ns) {
      da->in_debounce = false;
      if (da->pending_value != da->stable_value) {
        int val = da->pending_value;
        LOGI("Debounce TIMER ABS %u: stable=%d pending=%d -> EMIT %d\n",
             i, da->stable_value, da->pending_value, val);
        da->stable_value = val;
        spin_unlock_irqrestore(&ctx->debounce_lock, flags);
        rgp_process_abs_event_internal(ctx, vdev, i, val);
        spin_lock_irqsave(&ctx->debounce_lock, flags);
      } else {
        LOGI("Debounce TIMER ABS %u: stable=%d == pending, no emit\n",
             i, da->stable_value);
      }
    } else {
      u64 remaining = debounce_ns - elapsed;
      if (!need_restart || remaining < next_expire_ns) {
        next_expire_ns = remaining;
        need_restart = true;
      }
    }
  }

  spin_unlock_irqrestore(&ctx->debounce_lock, flags);

  if (need_restart) {
    hrtimer_forward_now(timer, ns_to_ktime(next_expire_ns));
    return HRTIMER_RESTART;
  }

  return HRTIMER_NORESTART;
}

/* Asymmetric debounce: press emitted immediately to minimize input lag,
 * release deferred by debounce_ms so a transient bounce-back to press cancels
 * the pending release (filters firmware JVS glitches and physical bouncing
 * on release without penalizing tap latency). */
static bool rgp_debounce_key(struct gp_ctx *ctx, unsigned int code, int value)
{
  unsigned long flags;
  struct gp_debounce_key *dk;
  u64 now_ns = ktime_get_ns();
  u64 debounce_ns = (u64)debounce_ms * 1000000ULL;
  bool should_emit = false;

  if (debounce_ms == 0 || code >= KEY_CNT)
    return true;

  spin_lock_irqsave(&ctx->debounce_lock, flags);
  dk = &ctx->key_debounce[code];

  if (value == dk->stable_value) {
    /* Bounce-back to stable (or no-op): cancel any pending change. */
    dk->pending_value = value;
    dk->in_debounce = false;
    should_emit = false;
  } else if (value) {
    /* PRESS: emit immediately for minimum input lag. */
    dk->stable_value = value;
    dk->pending_value = value;
    dk->last_change_time = now_ns;
    dk->in_debounce = false;
    should_emit = true;
  } else {
    /* RELEASE: defer by debounce_ms; cancelled if press returns before. */
    dk->pending_value = value;
    if (!dk->in_debounce) {
      dk->last_change_time = now_ns;
      dk->in_debounce = true;
      if (!hrtimer_active(&ctx->debounce_timer))
        hrtimer_start(&ctx->debounce_timer, ns_to_ktime(debounce_ns), HRTIMER_MODE_REL);
    }
    should_emit = false;
  }

  spin_unlock_irqrestore(&ctx->debounce_lock, flags);
  return should_emit;
}

static bool rgp_debounce_abs(struct gp_ctx *ctx, unsigned int code, int value)
{
  unsigned long flags;
  struct gp_debounce_abs *da;
  u64 now_ns = ktime_get_ns();
  u64 debounce_ns = (u64)debounce_ms * 1000000ULL;
  bool should_emit = false;

  if (debounce_ms == 0 || code >= ABS_CNT)
    return true;

  spin_lock_irqsave(&ctx->debounce_lock, flags);
  da = &ctx->abs_debounce[code];

  if (!da->initialized) {
    da->stable_value = value;
    da->pending_value = value;
    da->initialized = true;
    da->last_change_time = now_ns;
    da->in_debounce = false;
    should_emit = true;
    LOGI("Debounce ABS %u: INIT value=%d -> EMIT\n", code, value);
  } else if (da->in_debounce) {
    LOGI("Debounce ABS %u: IN_DEBOUNCE stable=%d pending=%d->%d -> SKIP\n",
         code, da->stable_value, da->pending_value, value);
    da->pending_value = value;
    should_emit = false;
  } else if (da->stable_value == value) {
    should_emit = false;
  } else {
    LOGI("Debounce ABS %u: NEW stable=%d->%d -> EMIT + START_DEBOUNCE\n",
         code, da->stable_value, value);
    da->stable_value = value;
    da->pending_value = value;
    da->last_change_time = now_ns;
    da->in_debounce = true;
    should_emit = true;

    if (!hrtimer_active(&ctx->debounce_timer))
      hrtimer_start(&ctx->debounce_timer, ns_to_ktime(debounce_ns), HRTIMER_MODE_REL);
  }

  spin_unlock_irqrestore(&ctx->debounce_lock, flags);
  return should_emit;
}

static bool rgp_any_abs_mode_active(struct gp_ctx *ctx)
{
  int i;
  for (i = 0; i < ABS_CNT; i++) {
    if (ctx->abs_mode_active[i])
      return true;
  }
  return false;
}

/* Internal processing functions (called after debounce) */
static void rgp_process_key_event_internal(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value)
{
  unsigned long flags;
  unsigned int modk = rgp_modifier_key();
  bool start_chord_mode = (rgp_combo_mode() == RGP_COMBO_MOD_START);
  bool should_fire_thumbstick_exit = false;
  struct rgp_rule rt_rules[RGP_MAX_COMBOS];
  int rt_n, i;

  if (code >= KEY_CNT) {
    LOGW("Invalid key code %u\n", code);
    return;
  }
  if (value > 1)
    value = 1;

  /* Snapshot runtime combo rules now (no lock held) so the firing loop below
   * can run them alongside the static rules without nesting the combos lock. */
  rt_n = rgp_snapshot_runtime_rules(rt_rules);

  /* Trace BTN_START at the very entry of the processing pipeline so we
   * can tell whether the source even delivers the event. */
  if (code == BTN_START)
    TAPDBG("BTN_START received at process_key code=%u value=%d combos=%d\n",
            code, value, rgp_combos_enabled());

  spin_lock_irqsave(&ctx->lock, flags);
  if (value) __set_bit(code, ctx->keys_down);
  else __clear_bit(code, ctx->keys_down);

  if (enable_thumbstick_exit && !ctx->exit_combo_active) {
    if (code == BTN_THUMBL) ctx->thumbl_down = (value != 0);
    else if (code == BTN_THUMBR) ctx->thumbr_down = (value != 0);

    if (value && ctx->thumbl_down && ctx->thumbr_down && !ctx->thumbstick_exit_fired) {
      ctx->thumbstick_exit_fired = true;
      ctx->exit_combo_active = true;
      should_fire_thumbstick_exit = true;
    }
    if (!ctx->thumbl_down && !ctx->thumbr_down)
      ctx->thumbstick_exit_fired = false;
  }
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (should_fire_thumbstick_exit) {
    LOGI("Thumbstick exit combo triggered (L3+R3)\n");
    rgp_emit_exit_combo(ctx, vdev);
    return;
  }

  if (enable_thumbstick_exit && ctx->thumbstick_exit_fired &&
      (code == BTN_THUMBL || code == BTN_THUMBR))
    return;

  spin_lock_irqsave(&ctx->lock, flags);

  if (!rgp_combos_enabled()) {
    spin_unlock_irqrestore(&ctx->lock, flags);
    if (test_bit(code, vdev->keybit)) {
      if (code == BTN_START)
        TAPDBG("BTN_START forward (combos disabled) value=%d\n", value);
      rgp_forward_raw_key_sync(ctx, vdev, code, value);
    } else if (code == BTN_START) {
      TAPDBG("BTN_START dropped: not in vdev->keybit (combos disabled)\n");
    }
    return;
  }

  if (code == modk) {
    if (value) {
      bool coalesced = ctx->mod_tap_pending_press;
      ctx->mod_pending = true;
      ctx->mod_used = false;
      ctx->mod_press_ts_ns = ktime_get_ns();
      bitmap_zero(ctx->mode_chord_down, KEY_CNT);
      ctx->mode_chord_count = 0;
      /* Coalesce only the still-pending autorepeat tap. A real fast tap
       * was already emitted at release time (press<80ms branch below),
       * so this only kills the long-press scheduled tap. */
      ctx->mod_tap_pending_press = false;
      if (enable_longpress_exit && start_longpress_ms > 0)
        mod_delayed_work(gp_wq, &ctx->mod_long_work, msecs_to_jiffies(start_longpress_ms));
      spin_unlock_irqrestore(&ctx->lock, flags);
      cancel_delayed_work(&ctx->mod_tap_press_work);
      TAPDBG("modifier PRESS code=%u (mod_pending=1, mod_used=0%s)\n",
              code, coalesced ? ", autorepeat coalesced" : "");
      return;
    } else {
      bool pending = ctx->mod_pending;
      bool used = ctx->mod_used;
      TAPDBG("modifier RELEASE code=%u pending=%d used=%d -> %s\n",
              code, pending, used,
              (pending && !used) ? "TAP" : "DROPPED");
      bool exit_active = ctx->exit_combo_active;
      bool need_mode_release = false;
      int abs_code;

      ctx->mod_pending = false;
      cancel_delayed_work(&ctx->mod_long_work);

      if (start_chord_mode && ctx->mode_chord_count > 0) {
        need_mode_release = true;
        bitmap_zero(ctx->mode_chord_down, KEY_CNT);
        ctx->mode_chord_count = 0;
      }

      for (abs_code = 0; abs_code < ABS_CNT; abs_code++) {
        if (ctx->abs_mode_active[abs_code]) {
          need_mode_release = true;
          ctx->abs_mode_active[abs_code] = false;
        }
      }

      if (!exit_active)
        cancel_delayed_work(&ctx->exit_combo_release_work);

      spin_unlock_irqrestore(&ctx->lock, flags);

      if (need_mode_release)
        rgp_emit_hold_key_sync(ctx, vdev, BTN_MODE, 0);

      if (pending && !used && modk < KEY_CNT) {
        /* Schedule the tap via the delayed-work pair (press now, release
         * after modifier_tap_delay_ms). A direct rgp_emit_tap_key would
         * emit press+release in the same evdev frame, and SDL/ES would
         * only see the final "released" state, missing the press entirely.
         * modifier_tap_delay_ms is the simulated hold duration on the
         * vdev — NOT an input lag, since ES processes the press as soon
         * as it lands. */
        u64 press_dur_ns = ktime_get_ns() - ctx->mod_press_ts_ns;
        TAPDBG("tap scheduled (press_dur=%llums, hold=%ums)\n",
                div_u64(press_dur_ns, NSEC_PER_MSEC), modifier_tap_delay_ms);
        spin_lock_irqsave(&ctx->lock, flags);
        ctx->mod_tap_pending_press = true;
        spin_unlock_irqrestore(&ctx->lock, flags);
        queue_delayed_work(gp_wq, &ctx->mod_tap_press_work, 0);
      }
      return;
    }
  }

  if (!value && test_bit(code, ctx->remapped_down)) {
    unsigned int out = ctx->remap_out[code];
    bool need_mode_release = false, was_mode_chord = false;

    __clear_bit(code, ctx->remapped_down);
    ctx->remap_out[code] = 0;

    if (start_chord_mode && test_bit(code, ctx->mode_chord_down)) {
      was_mode_chord = true;
      __clear_bit(code, ctx->mode_chord_down);
      if (ctx->mode_chord_count > 0) ctx->mode_chord_count--;
      if (ctx->mode_chord_count == 0 && !rgp_any_abs_mode_active(ctx))
        need_mode_release = true;
    }
    if (ctx->mod_pending) {
      ctx->mod_used = true;
      TAPDBG("mod_used=1 cause=key_remap_release code=%u out=%u\n", code, out);
    }
    spin_unlock_irqrestore(&ctx->lock, flags);

    if (out && test_bit(out, vdev->keybit)) {
      if (need_mode_release && was_mode_chord)
        rgp_emit_two_keys_sync(ctx, vdev, out, 0, BTN_MODE, 0);
      else
        rgp_emit_hold_key_sync(ctx, vdev, out, 0);
    } else if (need_mode_release && was_mode_chord) {
      rgp_emit_hold_key_sync(ctx, vdev, BTN_MODE, 0);
    }
    return;
  }

  if (value) {
    /* Runtime combos FIRST so a per-game combo wins over a static rule on the
     * same trigger (e.g. START+UP -> VIEW overrides the built-in volume rule). */
    for (i = 0; i < rt_n; i++) {
      const struct rgp_rule *r = &rt_rules[i];
      if (r->type != RGP_RULE_KEY || r->trig_code != code)
        continue;
      if (!rgp_mods_down(ctx, r))
        continue;

      if (ctx->mod_pending) {
        ctx->mod_used = true;
        TAPDBG("mod_used=1 cause=combo_key code=%u rule_out=%u\n", code, r->out_code);
      }

      if (r->out_tap) {
        __set_bit(code, ctx->remapped_down);
        ctx->remap_out[code] = 0;
        spin_unlock_irqrestore(&ctx->lock, flags);
        if (test_bit(r->out_code, vdev->keybit))
          rgp_emit_tap_key(vdev, r->out_code);
        return;
      }

      if (r->out_code < KEY_CNT) {
        __set_bit(code, ctx->remapped_down);
        ctx->remap_out[code] = (u16)r->out_code;
      }
      spin_unlock_irqrestore(&ctx->lock, flags);
      if (test_bit(r->out_code, vdev->keybit))
        rgp_emit_hold_key_sync(ctx, vdev, r->out_code, 1);
      return;
    }

    for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
      const struct rgp_rule *r = &rgp_rules[i];
      if (r->type != RGP_RULE_KEY || r->trig_code != code)
        continue;
      if (!rgp_rule_enabled(r))
        continue;
      if (!rgp_mods_down(ctx, r))
        continue;

      if (ctx->mod_pending) {
        ctx->mod_used = true;
        TAPDBG("mod_used=1 cause=key_rule code=%u rule_out=%u\n", code, r->out_code);
      }

      if (r->out_tap) {
        __set_bit(code, ctx->remapped_down);
        ctx->remap_out[code] = 0;
        spin_unlock_irqrestore(&ctx->lock, flags);
        if (test_bit(r->out_code, vdev->keybit))
          rgp_emit_tap_key(vdev, r->out_code);
        return;
      }

      if (r->out_code < KEY_CNT) {
        __set_bit(code, ctx->remapped_down);
        ctx->remap_out[code] = (u16)r->out_code;
      }
      spin_unlock_irqrestore(&ctx->lock, flags);
      if (test_bit(r->out_code, vdev->keybit))
        rgp_emit_hold_key_sync(ctx, vdev, r->out_code, 1);
      return;
    }

    if (enable_shift_mode_passthrough &&
        start_chord_mode && modk == BTN_START && ctx->mod_pending &&
        test_bit(BTN_START, ctx->keys_down) && !rgp_is_key_rule_trigger(code) &&
        /* BTN_MODE / BTN_SELECT are not game buttons: pressing one while the
         * SHIFT modifier (START) is held has no "hotkey" meaning, and BTN_MODE
         * is already the synthetic hotkey we'd emit — so START+MODE would just
         * become MODE+MODE. Some JVS firmwares send their "exit game" combo
         * exactly as "START held + MODE tap"; if we shift-passthrough it, START
         * gets consumed and never reaches the proxy, so RetroArch sees MODE
         * without START and the exit never fires. Forward these raw instead. */
        code != BTN_MODE && code != BTN_SELECT &&
        test_bit(code, vdev->keybit)) {
      bool need_mode_press = false;

      ctx->mod_used = true;
      TAPDBG("mod_used=1 cause=key_passthrough code=%u\n", code);
      if (!test_bit(code, ctx->mode_chord_down)) {
        __set_bit(code, ctx->mode_chord_down);
        ctx->mode_chord_count++;
        if (ctx->mode_chord_count == 1 && !rgp_any_abs_mode_active(ctx))
          need_mode_press = true;
      }
      __set_bit(code, ctx->remapped_down);
      ctx->remap_out[code] = (u16)code;
      spin_unlock_irqrestore(&ctx->lock, flags);

      if (need_mode_press)
        rgp_emit_two_keys_sync(ctx, vdev, BTN_MODE, 1, code, 1);
      else
        rgp_emit_hold_key_sync(ctx, vdev, code, 1);
      return;
    }
  }

  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!test_bit(code, vdev->keybit))
    return;

  rgp_forward_raw_key_sync(ctx, vdev, code, value);
}

static void rgp_process_abs_event_internal(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value)
{
  unsigned long flags;
  unsigned int modk = rgp_modifier_key();
  bool mod_down = false;
  bool mod_pending_now = false;
  bool start_chord_mode = (rgp_combo_mode() == RGP_COMBO_MOD_START);
  bool has_enabled_rules;
  bool use_hat_exact = false;
  struct rgp_rule rt_rules[RGP_MAX_COMBOS];
  int rt_n;
  int thr = 0, dir = 0, i;

  if (code >= ABS_CNT || !test_bit(code, vdev->absbit))
    return;

  if (!rgp_combos_enabled()) {
    input_event(vdev, EV_ABS, code, value);
    input_sync(vdev);
    return;
  }

  rt_n = rgp_snapshot_runtime_rules(rt_rules);
  has_enabled_rules = rgp_abs_has_enabled_rules(code);

  thr = rgp_abs_get_threshold(code, &use_hat_exact);
  if (thr == 0) {
    if (code >= ABS_HAT0X && code <= ABS_HAT3Y)
      use_hat_exact = true;
    else
      thr = stick_threshold;
  }

  if (!use_hat_exact && vdev->absinfo) {
    const struct input_absinfo *ai = &vdev->absinfo[code];
    if (ai->minimum == -1 && ai->maximum == 1) {
      use_hat_exact = true;
      LOGI("ABS axis %u detected as digital (-1/0/+1)\n", code);
    }
  }

  spin_lock_irqsave(&ctx->lock, flags);
  if (modk < KEY_CNT) mod_down = test_bit(modk, ctx->keys_down);
  mod_pending_now = ctx->mod_pending;

  LOGI("ABS event: code=%u value=%d mod_down=%d mod_pending=%d\n",
       code, value, mod_down, mod_pending_now);

  if (!mod_down) {
    bool need_mode_release = ctx->abs_mode_active[code];
    bool keys_have_mode;
    u16 combo_rel;
    /* Modifier-less combos (plain remaps) are gated out of the rule engine below
     * (which runs only while the modifier is held): handle them here so a JVS
     * lever/pedal mapped straight to a button works without a modifier. Engage on
     * entering the mapped direction, release on leaving it. */
    int fdir = rgp_abs_forward_dir(value, use_hat_exact, thr, stick_threshold);
    if (ctx->abs_combo_nomod[code] || fdir != ctx->abs_dir_state[code]) {
      const struct rgp_rule *m0 = NULL;
      for (i = 0; i < rt_n; i++) {
        if (rt_rules[i].type == RGP_RULE_ABS_DIR &&
            rgp_abs_rule_fires(rt_rules[i].nmods, rt_rules[i].trig_code,
                               rt_rules[i].trig_dir, code, fdir, false)) {
          m0 = &rt_rules[i];
          break;
        }
      }
      if (m0 || ctx->abs_combo_nomod[code]) {
        u16 rel = ctx->abs_combo_out[code];
        u16 press = m0 ? (u16)m0->out_code : 0;
        ctx->abs_dir_state[code] = fdir;
        ctx->abs_combo_out[code] = press;
        ctx->abs_combo_nomod[code] = (m0 != NULL);
        spin_unlock_irqrestore(&ctx->lock, flags);
        if (rel && rel != press)
          rgp_emit_hold_key_sync(ctx, vdev, rel, 0);
        if (press && press != rel && test_bit(press, vdev->keybit))
          rgp_emit_hold_key_sync(ctx, vdev, press, 1);
        return;
      }
    }
    /* Track the dir actually forwarded to the vdev (not 0): otherwise, if this
     * axis is held non-zero and the modifier is then pressed, the later release
     * (dir 0) matches a stale abs_dir_state=0 and gets deduped away, leaving the
     * forwarded value stuck down on the proxy. */
    ctx->abs_mode_active[code] = false;
    ctx->abs_dir_state[code] = rgp_abs_forward_dir(value, use_hat_exact, thr, stick_threshold);
    keys_have_mode = (ctx->mode_chord_count > 0);
    /* Modifier is gone: release any hold-combo this axis was holding. */
    combo_rel = ctx->abs_combo_out[code];
    ctx->abs_combo_out[code] = 0;
    spin_unlock_irqrestore(&ctx->lock, flags);

    if (combo_rel)
      rgp_emit_hold_key_sync(ctx, vdev, combo_rel, 0);

    if (need_mode_release && !rgp_any_abs_mode_active(ctx) && !keys_have_mode)
      rgp_emit_hold_key_sync(ctx, vdev, BTN_MODE, 0);

    input_event(vdev, EV_ABS, code, value);
    input_sync(vdev);
    return;
  }

  if (use_hat_exact) {
    if (value == -1) dir = -1;
    else if (value == 1) dir = +1;
    else dir = 0;
  } else {
    int down_thr = thr;
    int up_thr = thr;
    int prev_dir = ctx->abs_dir_state[code];
    if (down_thr <= 0) down_thr = stick_threshold;
    if (up_thr <= 0) up_thr = stick_threshold;
    /* Hysteresis: while the modifier is held, raise the engagement
     * threshold so analog tremor around the regular threshold cannot
     * mark the modifier as "used" (which steals the tap-on-release).
     * Disengage still uses the regular threshold.
     * Skipped for DT credit-abs codes: their thresholds are sized for
     * 0..255 pedal axes, where chord_stick_threshold (centered-stick
     * scale, default 24000) would be unreachable. */
    if (mod_pending_now && chord_stick_threshold > up_thr) {
      bool is_credit_abs = false;
      if (enable_shift_credit) {
        int k;
        for (k = 0; k < rgp_credit_abs_count; k++)
          if (rgp_credit_abs_rules[k].code == code) { is_credit_abs = true; break; }
      }
      if (!is_credit_abs)
        up_thr = chord_stick_threshold;
    }

    if (prev_dir > 0) {
      if (value <= -up_thr) dir = -1;
      else if (value >= down_thr) dir = +1;
      else dir = 0;
    } else if (prev_dir < 0) {
      if (value >= up_thr) dir = +1;
      else if (value <= -down_thr) dir = -1;
      else dir = 0;
    } else {
      if (value <= -up_thr) dir = -1;
      else if (value >= up_thr) dir = +1;
      else dir = 0;
    }
  }

  LOGI("ABS event: dir=%d prev_dir=%d thr=%d\n", dir, ctx->abs_dir_state[code], thr);

  if (dir == ctx->abs_dir_state[code]) {
    spin_unlock_irqrestore(&ctx->lock, flags);
    return;
  }
  ctx->abs_dir_state[code] = dir;

  /* The axis left the direction that engaged a hold-combo (we only reach here
   * on a dir change): release the held virtual key. */
  if (ctx->abs_combo_out[code]) {
    u16 rel = ctx->abs_combo_out[code];
    ctx->abs_combo_out[code] = 0;
    spin_unlock_irqrestore(&ctx->lock, flags);
    rgp_emit_hold_key_sync(ctx, vdev, rel, 0);
    return;
  }

  /* Runtime combos FIRST so a per-game combo wins over a static rule on the
   * same axis direction (e.g. START+UP -> VIEW overrides the volume rule). */
  if (dir != 0) {
    for (i = 0; i < rt_n; i++) {
      const struct rgp_rule *r = &rt_rules[i];
      if (r->type != RGP_RULE_ABS_DIR || r->trig_code != code || r->trig_dir != dir)
        continue;
      if (!rgp_mods_down(ctx, r))
        continue;

      LOGI("ABS combo matched: code=%u dir=%d -> out=%u\n", code, dir, r->out_code);
      if (ctx->mod_pending) {
        ctx->mod_used = true;
        TAPDBG("mod_used=1 cause=combo_abs code=%u dir=%d rule_out=%u\n",
                code, dir, r->out_code);
      }
      if (!r->out_tap)
        /* hold: keep the virtual key down until the axis leaves this
         * direction (released in the dir-change / mod-off paths below). */
        ctx->abs_combo_out[code] = (u16)r->out_code;
      spin_unlock_irqrestore(&ctx->lock, flags);
      if (test_bit(r->out_code, vdev->keybit)) {
        if (r->out_tap)
          rgp_emit_tap_key(vdev, r->out_code);
        else
          rgp_emit_hold_key_sync(ctx, vdev, r->out_code, 1);
      }
      return;
    }
  }

  if (has_enabled_rules && dir != 0) {
    for (i = 0; i < ARRAY_SIZE(rgp_rules); i++) {
      const struct rgp_rule *r = &rgp_rules[i];
      if (r->type != RGP_RULE_ABS_DIR || r->trig_code != code ||
          r->trig_dir != dir)
        continue;
      if (!rgp_rule_enabled(r))
        continue;
      if (!rgp_mods_down(ctx, r))
        continue;

      LOGI("ABS rule matched: code=%u dir=%d -> out=%u\n", code, dir, r->out_code);
      if (ctx->mod_pending) {
        ctx->mod_used = true;
        TAPDBG("mod_used=1 cause=abs_rule code=%u dir=%d rule_out=%u\n",
                code, dir, r->out_code);
      }
      spin_unlock_irqrestore(&ctx->lock, flags);
      if (r->out_tap && test_bit(r->out_code, vdev->keybit))
        rgp_emit_tap_key(vdev, r->out_code);
      return;
    }
  }

  if (enable_shift_credit && dir != 0) {
    for (i = 0; i < rgp_credit_abs_count; i++) {
      const struct rgp_credit_abs *cr = &rgp_credit_abs_rules[i];
      if (cr->code != code || cr->dir != dir)
        continue;
      LOGI("ABS credit-abs matched: code=%u dir=%d -> BTN_SELECT\n", code, dir);
      if (ctx->mod_pending) {
        ctx->mod_used = true;
        TAPDBG("mod_used=1 cause=credit_abs code=%u dir=%d\n",
                code, dir);
      }
      spin_unlock_irqrestore(&ctx->lock, flags);
      if (test_bit(BTN_SELECT, vdev->keybit)) {
        /* Press now, release after a short hold so cores reliably register
         * the coin (back-to-back press+release like rgp_emit_tap_key was
         * too fast for some emulators). */
        cancel_delayed_work(&ctx->credit_tap_release_work);
        input_report_key(vdev, BTN_SELECT, 1);
        input_sync(vdev);
        queue_delayed_work(gp_wq, &ctx->credit_tap_release_work,
                           msecs_to_jiffies(RGP_CREDIT_TAP_HOLD_MS));
      }
      return;
    }
  }

  if (enable_shift_mode_passthrough && start_chord_mode && modk == BTN_START && mod_pending_now) {
    bool need_mode_press = false;
    bool need_mode_release = false;
    bool any_mode_active;

    ctx->mod_used = true;
    TAPDBG("mod_used=1 cause=abs_passthrough code=%u dir=%d value=%d\n",
            code, dir, value);

    if (dir != 0) {
      if (!ctx->abs_mode_active[code]) {
        ctx->abs_mode_active[code] = true;
        any_mode_active = rgp_any_abs_mode_active(ctx);
        if (any_mode_active) {
          int cnt = 0;
          for (i = 0; i < ABS_CNT; i++)
            if (ctx->abs_mode_active[i]) cnt++;
          if (cnt == 1) any_mode_active = false;
        }
        if (!any_mode_active && ctx->mode_chord_count == 0)
          need_mode_press = true;
      }
    } else {
      if (ctx->abs_mode_active[code]) {
        ctx->abs_mode_active[code] = false;
        if (!rgp_any_abs_mode_active(ctx) && ctx->mode_chord_count == 0)
          need_mode_release = true;
      }
    }

    spin_unlock_irqrestore(&ctx->lock, flags);

    if (need_mode_press)
      rgp_emit_hold_key_sync(ctx, vdev, BTN_MODE, 1);

    input_event(vdev, EV_ABS, code, value);

    if (need_mode_release)
      rgp_emit_hold_key_sync(ctx, vdev, BTN_MODE, 0);
    else
      input_sync(vdev);

    return;
  }

  spin_unlock_irqrestore(&ctx->lock, flags);
  input_event(vdev, EV_ABS, code, value);
  input_sync(vdev);
}

/* Public entry points with debouncing */
static void rgp_process_key_event(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value)
{
  if (rgp_debounce_key(ctx, code, value))
    rgp_process_key_event_internal(ctx, vdev, code, value);
}

static void rgp_process_abs_event(struct gp_ctx *ctx, struct input_dev *vdev, unsigned int code, int value)
{
  if (rgp_debounce_abs(ctx, code, value))
    rgp_process_abs_event_internal(ctx, vdev, code, value);
}

static void gp_create_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(work, struct gp_ctx, create_work);
  struct input_dev *src = ctx->handle.dev, *vdev;
  unsigned long flags;
  DECLARE_BITMAP(extra_keys, KEY_CNT);
  int err, code;
  const char *src_name;

  if (atomic_read(&ctx->closing)) {
    LOGI("create_work: closing=1 -> skip\n");
    goto out_put;
  }

  src_name = src->name ? src->name : "Gamepad";
  ctx->vname = kasprintf(GFP_KERNEL, "%s %s", src_name, ACRONYM);
  if (!ctx->vname) {
    LOGE("create_work: kasprintf(vname) failed\n");
    goto out_put;
  }

  ctx->vphys = kasprintf(GFP_KERNEL, "%s/%s", DRV_NAME, ACRONYM);
  if (!ctx->vphys) {
    LOGE("create_work: kasprintf(vphys) failed\n");
    goto out_free_name;
  }

  LOGI("create_work: src=\"%s\" -> vname=\"%s\"\n", src_name, ctx->vname);

  vdev = input_allocate_device();
  if (!vdev) {
    LOGE("create_work: input_allocate_device failed\n");
    goto out_free_phys;
  }

  vdev->name = ctx->vname;
  vdev->phys = ctx->vphys;
  vdev->id.bustype = BUS_VIRTUAL;
  vdev->id.vendor = VENDOR_VIRT;
  vdev->id.product = PRODUCT_VIRT;
  vdev->dev.parent = ctx->parent_dev;

  __set_bit(EV_SYN, vdev->evbit);
  __set_bit(EV_KEY, vdev->evbit);
  __set_bit(EV_ABS, vdev->evbit);

  for (code = BTN_MISC; code < KEY_CNT; code++)
    if (test_bit(code, src->keybit))
      __set_bit(code, vdev->keybit);

  __set_bit(BTN_START, vdev->keybit);
  __set_bit(BTN_SELECT, vdev->keybit);
  __set_bit(BTN_MODE, vdev->keybit);
  __set_bit(KEY_VOLUMEUP, vdev->keybit);
  __set_bit(KEY_VOLUMEDOWN, vdev->keybit);

  rgp_collect_external_target_keybits(extra_keys);
  bitmap_or(vdev->keybit, vdev->keybit, extra_keys, KEY_CNT);

  for (code = 0; code < ABS_CNT; code++) {
    if (!test_bit(code, src->absbit))
      continue;
    if (src->absinfo) {
      const struct input_absinfo *ai = &src->absinfo[code];
      input_set_abs_params(vdev, code, ai->minimum, ai->maximum, ai->fuzz, ai->flat);
    } else {
      int min, max;
      default_abs_range(code, &min, &max);
      input_set_abs_params(vdev, code, min, max, 0, 0);
    }
  }

  err = input_register_device(vdev);
  if (err) {
    LOGE("create_work: input_register_device failed err=%d\n", err);
    input_free_device(vdev);
    goto out_free_phys;
  }

  /* Mirror the source's rgp_uid onto the proxy so the frontend can pair
   * source ↔ proxy by UID. Auto-cleaned by input_unregister_device. */
  if (device_create_file(&vdev->dev, &dev_attr_rgp_uid) != 0)
    LOGW("create_work: failed to create rgp_uid on vdev (non-fatal)\n");

  spin_lock_irqsave(&ctx->lock, flags);
  if (atomic_read(&ctx->closing)) {
    ctx->vdev_dead = vdev;
    spin_unlock_irqrestore(&ctx->lock, flags);
    LOGI("create_work: created but closing -> queue destroy\n");
    ctx_get(ctx);
    queue_work(gp_wq, &ctx->destroy_work);
    goto out_put;
  }
  ctx->vdev = vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  pr_info(DRV_NAME ": virtual device registered (%s) v%s\n", dev_name(&vdev->dev), DRV_VERSION);
  goto out_put;

out_free_phys:
  kfree(ctx->vphys);
  ctx->vphys = NULL;
out_free_name:
  kfree(ctx->vname);
  ctx->vname = NULL;
out_put:
  ctx_put(ctx);
}

static void gp_destroy_workfn(struct work_struct *work)
{
  struct gp_ctx *ctx = container_of(work, struct gp_ctx, destroy_work);
  struct input_dev *dead = NULL;
  unsigned long flags;

  spin_lock_irqsave(&ctx->lock, flags);
  dead = ctx->vdev_dead;
  ctx->vdev_dead = NULL;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (dead) {
    LOGI("destroy_work: input_unregister_device...\n");
    input_unregister_device(dead);
    LOGI("destroy_work: done\n");
  }

  kfree(ctx->vname);
  ctx->vname = NULL;
  kfree(ctx->vphys);
  ctx->vphys = NULL;

  if (ctx->parent_dev) {
    put_device(ctx->parent_dev);
    ctx->parent_dev = NULL;
  }

  ctx_put(ctx);
}

static void gp_event(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
  struct gp_ctx *ctx = handle->private;
  struct input_dev *vdev;
  unsigned long flags;
  int effective_value = value;

  if (atomic_read(&ctx->closing))
    return;

  /* Passive ctx: the device is tagged with rgp_uid but not proxified.
   * No proxy event processing, no input transform — let the source's
   * events flow untouched to userspace. */
  if (ctx->passive)
    return;

  effective_value = rgp_apply_source_input_transform(handle->dev, type, code, value);

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (!vdev)
    return;

  switch (type) {
    case EV_KEY:
      if (rgp_should_replace_target_event(ctx, type, code, effective_value)) {
        LOGI("Replacing target event on \"%s\": code=%u value=%d (clearing raw_forward)\n",
             handle->dev->name ? handle->dev->name : "(null)", code, effective_value);
        /* The source's raw value for this code must NOT leak into vkeys_down,
         * otherwise a stuck-pressed source (e.g. JVS lightgun cabs wire the
         * holster sensor to BTN_EAST: the gun-in-holster state holds A=1 in
         * the firmware indefinitely) would pin out_value=1 forever and any
         * external binding pointing at the same dst would never produce a
         * transition. Force-clear raw_forward_down[code] and replay the
         * resulting logical state so the vdev reflects only what the
         * binding is doing. */
        rgp_forward_raw_key_sync(ctx, vdev, code, 0);
      } else if (rgp_should_replace_source_event(handle->dev, type, code, effective_value)) {
        LOGI("Replacing source event on \"%s\": code=%u value=%d\n",
             handle->dev->name ? handle->dev->name : "(null)", code, effective_value);
      } else {
        rgp_process_key_event(ctx, vdev, code, effective_value);
      }
      break;
    case EV_ABS:
      rgp_process_abs_event(ctx, vdev, code, effective_value);
      break;
    case EV_SYN:
      break;
    default:
      input_event(vdev, type, code, effective_value);
      input_sync(vdev);
      break;
  }

  rgp_process_external_bindings(handle->dev, type, code, effective_value);
}

static int gp_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id)
{
  struct gp_ctx *ctx;
  bool match;
  int err;

  LOGI("connect? dev=%s name=\"%s\"\n", dev_name(&dev->dev), dev->name ? dev->name : "(null)");

  if (!is_likely_gamepad(dev))
    return -ENODEV;

  match = rgp_device_name_matches(dev->name);

  ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
  if (!ctx)
    return -ENOMEM;

  kref_init(&ctx->ref);
  atomic_set(&ctx->closing, 0);
  spin_lock_init(&ctx->lock);
  spin_lock_init(&ctx->debounce_lock);
  INIT_LIST_HEAD(&ctx->node);

  ctx->uid = rgp_get_or_alloc_uid(dev);
  ctx->passive = !match;

  bitmap_zero(ctx->keys_down, KEY_CNT);
  bitmap_zero(ctx->vkeys_down, KEY_CNT);
  bitmap_zero(ctx->raw_forward_down, KEY_CNT);
  memset(ctx->synth_hold_count, 0, sizeof(ctx->synth_hold_count));
  bitmap_zero(ctx->remapped_down, KEY_CNT);
  memset(ctx->remap_out, 0, sizeof(ctx->remap_out));
  bitmap_zero(ctx->mode_chord_down, KEY_CNT);
  ctx->mode_chord_count = 0;
  memset(ctx->abs_dir_state, 0, sizeof(ctx->abs_dir_state));
  memset(ctx->abs_mode_active, 0, sizeof(ctx->abs_mode_active));

  memset(ctx->key_debounce, 0, sizeof(ctx->key_debounce));
  memset(ctx->abs_debounce, 0, sizeof(ctx->abs_debounce));

  ctx->mod_pending = false;
  ctx->mod_used = false;
  ctx->exit_combo_active = false;
  ctx->thumbl_down = false;
  ctx->thumbr_down = false;
  ctx->thumbstick_exit_fired = false;
  ctx->sysfs_created = false;
  ctx->uid_attr_source_created = false;
  ctx->grabbed = false;

  hrtimer_init(&ctx->debounce_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  ctx->debounce_timer.function = rgp_debounce_timer_fn;

  INIT_WORK(&ctx->create_work, gp_create_workfn);
  INIT_WORK(&ctx->destroy_work, gp_destroy_workfn);
  INIT_DELAYED_WORK(&ctx->mod_long_work, rgp_mod_long_workfn);
  INIT_DELAYED_WORK(&ctx->exit_combo_release_work, rgp_exit_combo_release_workfn);
  INIT_DELAYED_WORK(&ctx->mod_tap_press_work, rgp_mod_tap_press_workfn);
  INIT_DELAYED_WORK(&ctx->mod_tap_release_work, rgp_mod_tap_release_workfn);
  INIT_DELAYED_WORK(&ctx->credit_tap_release_work, rgp_credit_tap_release_workfn);

  ctx->handle.dev = dev;
  ctx->handle.handler = handler;
  ctx->handle.name = DRV_NAME;
  ctx->handle.private = ctx;
  ctx->parent_dev = get_device(&dev->dev);

  err = input_register_handle(&ctx->handle);
  if (err) {
    LOGE("input_register_handle failed err=%d\n", err);
    goto fail_putdev;
  }
  LOGI("input_register_handle ok\n");

  err = input_open_device(&ctx->handle);
  if (err) {
    LOGE("input_open_device failed err=%d\n", err);
    goto fail_unreg_handle;
  }
  LOGI("input_open_device ok\n");

  /* Stamp UID on the source device for every recognized gamepad
   * (proxified or not). Frontend uses it to pair source ↔ proxy. */
  if (device_create_file(&dev->dev, &dev_attr_rgp_uid) == 0) {
    ctx->uid_attr_source_created = true;
    LOGI("rgp_uid=%u stamped on source \"%s\"\n",
         ctx->uid, dev->name ? dev->name : "(null)");
  } else {
    LOGW("device_create_file rgp_uid failed (non-fatal)\n");
  }

  if (match) {
    if (rgp_dt_filter_count || rgp_user_filter_count)
      LOGI("Device \"%s\" matches filter (uid=%u) -> proxify\n",
           dev->name ? dev->name : "(null)", ctx->uid);

    if (do_grab) {
      err = input_grab_device(&ctx->handle);
      if (err) {
        LOGE("input_grab_device failed err=%d\n", err);
        goto fail_close;
      }
      ctx->grabbed = true;
      LOGI("input_grab_device ok\n");

      err = device_create_file(&dev->dev, &dev_attr_rgp_proxied);
      if (err) {
        LOGW("device_create_file rgp_proxied failed err=%d (non-fatal)\n", err);
      } else {
        ctx->sysfs_created = true;
        LOGI("Created sysfs attribute rgp_proxied\n");
      }
      kobject_uevent(&dev->dev.kobj, KOBJ_CHANGE);
      LOGI("Sent KOBJ_CHANGE uevent\n");
    } else {
      LOGI("do_grab=0 -> not grabbing\n");
    }
  } else {
    LOGI("Device \"%s\" passive (uid=%u, no filter match)\n",
         dev->name ? dev->name : "(null)", ctx->uid);
  }

  write_lock(&rgp_ctx_list_lock);
  list_add_tail(&ctx->node, &rgp_ctx_list);
  write_unlock(&rgp_ctx_list_lock);

  if (match) {
    ctx_get(ctx);
    queue_work(gp_wq, &ctx->create_work);
  }

  pr_info(DRV_NAME ": %s %s uid=%u v%s\n",
          match ? "proxified" : "tagged",
          dev_name(&dev->dev), ctx->uid, DRV_VERSION);
  return 0;

fail_close:
  input_close_device(&ctx->handle);
fail_unreg_handle:
  input_unregister_handle(&ctx->handle);
fail_putdev:
  if (ctx->parent_dev) {
    put_device(ctx->parent_dev);
    ctx->parent_dev = NULL;
  }
  ctx_put(ctx);
  return err;
}

static void gp_disconnect(struct input_handle *handle)
{
  struct gp_ctx *ctx = handle->private;
  struct input_dev *vdev;
  struct input_dev *real_dev = handle->dev;
  unsigned long flags;
  int code;

  LOGI("disconnect begin dev=%s\n", dev_name(&handle->dev->dev));
  atomic_set(&ctx->closing, 1);

  write_lock(&rgp_ctx_list_lock);
  list_del_init(&ctx->node);
  write_unlock(&rgp_ctx_list_lock);

  cancel_delayed_work_sync(&ctx->mod_long_work);
  cancel_delayed_work_sync(&ctx->mod_tap_press_work);
  cancel_delayed_work_sync(&ctx->mod_tap_release_work);
  cancel_delayed_work_sync(&ctx->credit_tap_release_work);

  hrtimer_cancel(&ctx->debounce_timer);

  if (cancel_delayed_work_sync(&ctx->exit_combo_release_work)) {
    bool need_release = false;
    spin_lock_irqsave(&ctx->lock, flags);
    need_release = ctx->exit_combo_active && ctx->vdev;
    vdev = ctx->vdev;
    ctx->exit_combo_active = false;
    spin_unlock_irqrestore(&ctx->lock, flags);

    if (need_release && vdev)
      rgp_emit_two_keys_sync(ctx, vdev, BTN_MODE, 0, BTN_START, 0);
  }

  spin_lock_irqsave(&ctx->lock, flags);
  vdev = ctx->vdev;
  ctx->vdev = NULL;
  if (vdev)
    ctx->vdev_dead = vdev;
  spin_unlock_irqrestore(&ctx->lock, flags);

  if (vdev) {
    for (code = 0; code < KEY_CNT; code++) {
      if (test_bit(code, ctx->vkeys_down))
        input_report_key(vdev, code, 0);
    }
    input_sync(vdev);
  }

  if (ctx->sysfs_created) {
    device_remove_file(&real_dev->dev, &dev_attr_rgp_proxied);
    ctx->sysfs_created = false;
    LOGI("Removed sysfs attribute rgp_proxied\n");
    kobject_uevent(&real_dev->dev.kobj, KOBJ_CHANGE);
    LOGI("Sent KOBJ_CHANGE uevent (unproxied)\n");
  }

  if (ctx->uid_attr_source_created) {
    device_remove_file(&real_dev->dev, &dev_attr_rgp_uid);
    ctx->uid_attr_source_created = false;
    LOGI("Removed sysfs attribute rgp_uid (uid=%u)\n", ctx->uid);
  }

  if (ctx->grabbed)
    input_release_device(handle);
  input_close_device(handle);
  input_unregister_handle(handle);

  /* Evict the UID cache only on a real disconnect (unplug). When we are in
   * the middle of a rescan, keep the entry so the next gp_connect on the
   * same input_dev reuses the same UID. */
  if (!READ_ONCE(rgp_in_rescan))
    rgp_remove_uid_entry(real_dev);

  ctx_get(ctx);
  queue_work(gp_wq, &ctx->destroy_work);

  pr_info(DRV_NAME ": disconnected v%s\n", DRV_VERSION);
  ctx_put(ctx);
}

/* ============================================================================
 * AUXILIARY INPUT HANDLER (keyboards / mouse buttons / misc EV_KEY devices)
 * ============================================================================ */

static void rgp_aux_event(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
  int effective_value = value;

  if (type != EV_KEY)
    return;

  effective_value = rgp_apply_source_input_transform(handle->dev, type, code, value);
  rgp_process_external_bindings(handle->dev, type, code, effective_value);
}

static int rgp_aux_connect(struct input_handler *handler, struct input_dev *dev,
                           const struct input_device_id *id)
{
  struct rgp_aux_ctx *aux;
  int err;

  if (!is_external_source_candidate(dev))
    return -ENODEV;

  aux = kzalloc(sizeof(*aux), GFP_KERNEL);
  if (!aux)
    return -ENOMEM;

  aux->handle.dev = dev;
  aux->handle.handler = handler;
  aux->handle.name = DRV_NAME "_aux";
  aux->handle.private = aux;

  err = input_register_handle(&aux->handle);
  if (err) {
    kfree(aux);
    return err;
  }

  err = input_open_device(&aux->handle);
  if (err) {
    input_unregister_handle(&aux->handle);
    kfree(aux);
    return err;
  }

  LOGI("aux connected: dev=%s name=\"%s\"\n", dev_name(&dev->dev), dev->name ? dev->name : "(null)");
  return 0;
}

static void rgp_aux_disconnect(struct input_handle *handle)
{
  struct rgp_aux_ctx *aux = handle->private;

  LOGI("aux disconnected: dev=%s\n", dev_name(&handle->dev->dev));
  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(aux);
}

static const struct input_device_id gp_ids[] = {
  { .driver_info = 1 },
  { },
};
MODULE_DEVICE_TABLE(input, gp_ids);

static struct input_handler gp_handler = {
  .event = gp_event,
  .connect = gp_connect,
  .disconnect = gp_disconnect,
  .name = DRV_NAME,
  .id_table = gp_ids,
};

static const struct input_device_id rgp_aux_ids[] = {
  { .driver_info = 1 },
  { },
};
MODULE_DEVICE_TABLE(input, rgp_aux_ids);

static struct input_handler rgp_aux_handler = {
  .event = rgp_aux_event,
  .connect = rgp_aux_connect,
  .disconnect = rgp_aux_disconnect,
  .name = DRV_NAME "_aux",
  .id_table = rgp_aux_ids,
};

/* ============================================================================
 * PLATFORM DRIVER FOR DEVICE TREE
 * ============================================================================ */

static bool aux_handler_registered;

/* Re-scan all input devices after a runtime filter change.
 * Runs on the system workqueue: unregister + register the main input_handler,
 * which triggers gp_disconnect for currently proxified devices and gp_connect
 * for every device matching the new filter (including ones connected before
 * the filter was updated). */
static void rgp_filter_rescan_workfn(struct work_struct *w)
{
  int err;

  if (!READ_ONCE(main_handler_registered)) {
    LOGI("Filter rescan: main handler not registered, skipping\n");
    return;
  }

  pr_info(DRV_NAME ": rescanning input devices after filter change v%s\n", DRV_VERSION);

  /* Tell gp_disconnect() to keep UID-cache entries around: the imminent
   * re-register will call gp_connect() on the same input_devs and we want
   * each one to recover its previous UID. */
  WRITE_ONCE(rgp_in_rescan, true);

  input_unregister_handler(&gp_handler);
  WRITE_ONCE(main_handler_registered, false);

  err = input_register_handler(&gp_handler);
  if (err) {
    LOGE("Filter rescan: re-register failed err=%d\n", err);
    WRITE_ONCE(rgp_in_rescan, false);
    return;
  }
  WRITE_ONCE(main_handler_registered, true);
  WRITE_ONCE(rgp_in_rescan, false);
  LOGI("Filter rescan complete\n");
}

static int rgp_platform_probe(struct platform_device *pdev)
{
  int err;

  LOGI("Platform probe\n");
  rgp_parse_dt(pdev->dev.of_node);

  if (main_handler_registered || aux_handler_registered) {
    LOGI("Handlers already registered\n");
    return 0;
  }

  rgp_class = class_create(DRV_NAME);
  if (IS_ERR(rgp_class)) {
    LOGE("Failed to create sysfs class\n");
    return PTR_ERR(rgp_class);
  }

  err = rgp_create_class_files();
  if (err) {
    LOGE("Failed to create sysfs class files\n");
    class_destroy(rgp_class);
    return err;
  }

  gp_wq = alloc_workqueue(DRV_NAME "_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
  if (!gp_wq) {
    LOGE("Failed to allocate workqueue\n");
    rgp_remove_class_files();
    class_destroy(rgp_class);
    return -ENOMEM;
  }

  err = input_register_handler(&gp_handler);
  if (err) {
    LOGE("input_register_handler(main) failed err=%d\n", err);
    destroy_workqueue(gp_wq);
    rgp_remove_class_files();
    class_destroy(rgp_class);
    gp_wq = NULL;
    return err;
  }
  main_handler_registered = true;

  err = input_register_handler(&rgp_aux_handler);
  if (err) {
    LOGE("input_register_handler(aux) failed err=%d\n", err);
    input_unregister_handler(&gp_handler);
    main_handler_registered = false;
    destroy_workqueue(gp_wq);
    rgp_remove_class_files();
    class_destroy(rgp_class);
    gp_wq = NULL;
    return err;
  }
  aux_handler_registered = true;

  pr_info(DRV_NAME ": handlers registered via DT probe v%s\n", DRV_VERSION);
  pr_info(DRV_NAME ": Runtime config available at /sys/class/" DRV_NAME "/\n");
  return 0;
}

static void rgp_platform_remove(struct platform_device *pdev)
{
  LOGI("Platform remove\n");

  /* Drain any in-flight filter rescan before tearing down the handlers. */
  cancel_work_sync(&rgp_filter_rescan_work);

  if (aux_handler_registered) {
    input_unregister_handler(&rgp_aux_handler);
    aux_handler_registered = false;
  }

  if (main_handler_registered) {
    input_unregister_handler(&gp_handler);
    main_handler_registered = false;
  }

  if (gp_wq) {
    flush_workqueue(gp_wq);
    destroy_workqueue(gp_wq);
    gp_wq = NULL;
  }

  if (rgp_class) {
    rgp_remove_class_files();
    class_destroy(rgp_class);
    rgp_class = NULL;
  }

  pr_info(DRV_NAME ": handlers unregistered v%s\n", DRV_VERSION);
}

static const struct of_device_id rgp_of_match[] = {
  { .compatible = "recalbox,gamepad-proxy" },
  { },
};
MODULE_DEVICE_TABLE(of, rgp_of_match);

static struct platform_driver rgp_platform_driver = {
  .probe = rgp_platform_probe,
  .remove = rgp_platform_remove,
  .driver = {
    .name = DRV_NAME,
    .of_match_table = rgp_of_match,
  },
};

/* ============================================================================
 * MODULE INIT / EXIT
 * ============================================================================ */

static bool driver_active;

static int __init gp_init(void)
{
  struct device_node *np;
  int err;

  pr_info(DRV_NAME ": init v%s\n", DRV_VERSION);

  np = of_find_compatible_node(NULL, NULL, "recalbox,gamepad-proxy");
  if (!np) {
    pr_info(DRV_NAME ": no DT node, driver inactive\n");
    return 0;
  }
  of_node_put(np);

  if (!validate_rules()) {
    LOGE("Rule validation failed\n");
    return -EINVAL;
  }

  if (combo_modifier < RGP_COMBO_MOD_START || combo_modifier > RGP_COMBO_MOD_DISABLED) {
    LOGW("Invalid combo_modifier %d, using default\n", combo_modifier);
    combo_modifier = RGP_COMBO_MOD_START;
  }

  if (stick_threshold < 0 || stick_threshold > 32767) {
    LOGW("Invalid stick_threshold %d, using default\n", stick_threshold);
    stick_threshold = 16000;
  }

  if (exit_combo_release_ms < 10 || exit_combo_release_ms > 1000) {
    LOGW("Invalid exit_combo_release_ms %u, using default\n", exit_combo_release_ms);
    exit_combo_release_ms = 50;
  }

  if (modifier_tap_delay_ms > 5000) {
    LOGW("Invalid modifier_tap_delay_ms %u, using default\n", modifier_tap_delay_ms);
    modifier_tap_delay_ms = 100;
  }

  if (debounce_ms > 500) {
    LOGW("Invalid debounce_ms %u, using default (0=disabled)\n", debounce_ms);
    debounce_ms = 0;
  }

  err = rgp_apply_runtime_bindings_from_string(external_maps_cfg);
  if (err < 0) {
    LOGE("Invalid external_maps module parameter (%d)\n", err);
    return err;
  }

  err = rgp_apply_runtime_inverted_inputs_from_string(inverted_inputs_cfg);
  if (err < 0) {
    LOGE("Invalid inverted_inputs module parameter (%d)\n", err);
    return err;
  }

  rgp_user_filter_count = rgp_parse_filter_list(device_name_filter, rgp_user_filters);
  {
    int i;
    for (i = 0; i < rgp_user_filter_count; i++)
      LOGI("Module-param device_name[%d]=\"%s\"\n", i, rgp_user_filters[i]);
  }

  INIT_WORK(&rgp_filter_rescan_work, rgp_filter_rescan_workfn);

  err = platform_driver_register(&rgp_platform_driver);
  if (err) {
    LOGE("Failed to register platform driver err=%d\n", err);
    return err;
  }

  driver_active = true;
  pr_info(DRV_NAME ": platform driver registered, waiting for DT match v%s\n", DRV_VERSION);
  return 0;
}

static void __exit gp_exit(void)
{
  pr_info(DRV_NAME ": exit start v%s\n", DRV_VERSION);
  if (driver_active) {
    platform_driver_unregister(&rgp_platform_driver);
    driver_active = false;
  }
  rgp_uid_table_destroy();
  pr_info(DRV_NAME ": exit done v%s\n", DRV_VERSION);
}

module_init(gp_init);
module_exit(gp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@digitalLumberjack");
MODULE_DESCRIPTION("Recalbox Gamepad Proxy " ACRONYM " v" DRV_VERSION " with external source mappings and generic source input inversion support");
MODULE_VERSION(DRV_VERSION);