// SPDX-License-Identifier: GPL-2.0
/*
 * HID driver for RecalboxJVS arcade controller
 * - sysfs config attributes on platform device via HID Feature Reports
 */

#include <linux/module.h>
#include <linux/hid.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/workqueue.h>
#include <linux/reboot.h>
#include <linux/notifier.h>

/* Report 5: Configuration - written ONLY when user commits a change.
 * Each SET triggers a notify on firmware side, so DO NOT write periodically. */
#define RECALBOXJVS_CONFIG_REPORT_ID  5
#define RECALBOXJVS_CONFIG_SIZE       18

/* Config byte offsets - read/write (bytes 0..7) */
#define CFG_CAB_TYPE          0
#define CFG_ANALOG_ENABLED    1
#define CFG_LINK_MODE         2
#define CFG_PLAYERS_ON_PANEL  3   /* 0 = 1P, 1 = 2P */
#define CFG_FLIP_GUN_X        4   /* gun cab: invert X */
#define CFG_FLIP_GUN_Y        5   /* gun cab: invert Y */
#define CFG_START_QUICK_EXIT  6   /* 3 s hold → exit (vs menu) */
#define CFG_SKIP_BOOT_LOGO    7   /* boot straight to Pi5, no splash */

/* Action report - one-shot commands. Must match firmware-side USBManager.cpp. */
#define RECALBOXJVS_ACTION_REPORT_ID  10
#define RECALBOXJVS_ACTION_SIZE       2
#define ACTION_MAGIC                  0xA5
#define ACTION_RESET_CONFIG           0x01
#define ACTION_FLASH_MODE             0x02
#define ACTION_SHUTDOWN               0x03  /* cut amps + UAC + LEDs on the firmware */
#define ACTION_WAKE                   0x04  /* explicit wake (tests); prod wakes via USB re-enum */

/* Config byte offsets - read only (bytes 8..17) */
#define CFG_FIRMWARE_MAJOR          8
#define CFG_FIRMWARE_MINOR          9
#define CFG_IO_BOARD_COUNT          10
#define CFG_HW_REVISION             11  /* PCB revision burned in eFuse (1, 2, 3, ...) */
#define CFG_HDMI_ONLY               12  /* DIP switch: cabinet is HDMI-only (no VGA chassis) */
#define CFG_KHZ15_ONLY              13  /* DIP switch: cabinet is 15 kHz-only (no 31 kHz firmware UI) */
#define CFG_CAB_FAMILY              14  // CabFamily enum: 0=None 1=Joystick 2=Race 3=Gun
#define CFG_CAB_SCREEN_LOCK         15  // CabScreenLock enum: 0=none 1=khz15 2=khz31
#define CFG_CAB_DEFAULT_SCREENTYPE  16  // 0=auto 1=khz15 2=multi1525 3=khz31 4=multi1531 5=trifreq
// byte 17 reserved

/* Report 9: Telemetry - safe to write continuously (rpi temp, ...).
 * MUST NOT carry configuration data. */
#define RECALBOXJVS_TELEMETRY_REPORT_ID  9
#define RECALBOXJVS_TELEMETRY_SIZE       4

#define TLM_RPI5_TEMP       0
/* bytes 1..3 reserved for future telemetry */

/* Report 11: ESP→Pi5 video control. Byte 0: 1=BLANK, 2=UNBLANK. Forwards to rp1-dpi OMASK. */
#define RECALBOXJVS_VIDEO_REPORT_ID    11
#define VIDEO_CMD_BLANK                1
#define VIDEO_CMD_UNBLANK              2

extern int rp1dpi_blank_global(bool blank);

/* Global reference to the HID device */
static struct hid_device* recalboxjvs_hdev;
static struct delayed_work temp_work;

/* ---- Feature Report helpers ---- */

static int recalboxjvs_get_config(struct hid_device* hdev, u8* config)
{
  u8* buf;
  int ret;

  buf = kzalloc(RECALBOXJVS_CONFIG_SIZE + 1, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  buf[0] = RECALBOXJVS_CONFIG_REPORT_ID;
  ret = hid_hw_raw_request(hdev, RECALBOXJVS_CONFIG_REPORT_ID, buf,
                           RECALBOXJVS_CONFIG_SIZE + 1,
                           HID_FEATURE_REPORT, HID_REQ_GET_REPORT);
  if (ret > 1)
    memcpy(config, buf + 1, RECALBOXJVS_CONFIG_SIZE);
  else if (ret >= 0)
    ret = -EIO;

  kfree(buf);
  return ret > 0 ? 0 : ret;
}

static int recalboxjvs_set_config_byte(struct hid_device* hdev,
                                       int offset, u8 val)
{
  u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};
  u8* buf;
  int ret;

  ret = recalboxjvs_get_config(hdev, config);
  if (ret)
    return ret;

  if (offset < 0 || offset >= RECALBOXJVS_CONFIG_SIZE)
    return -EINVAL;

  config[offset] = val;

  buf = kzalloc(RECALBOXJVS_CONFIG_SIZE + 1, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  buf[0] = RECALBOXJVS_CONFIG_REPORT_ID;
  memcpy(buf + 1, config, RECALBOXJVS_CONFIG_SIZE);
  ret = hid_hw_raw_request(hdev, RECALBOXJVS_CONFIG_REPORT_ID, buf,
                           RECALBOXJVS_CONFIG_SIZE + 1,
                           HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

  kfree(buf);
  return ret > 0 ? 0 : ret;
}

/* Send a one-shot action command via the dedicated Action report. */
static int recalboxjvs_send_action(struct hid_device* hdev, u8 action)
{
  u8* buf;
  int ret;

  buf = kzalloc(RECALBOXJVS_ACTION_SIZE + 1, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  buf[0] = RECALBOXJVS_ACTION_REPORT_ID;
  buf[1] = action;
  buf[2] = ACTION_MAGIC;
  ret = hid_hw_raw_request(hdev, RECALBOXJVS_ACTION_REPORT_ID, buf,
                           RECALBOXJVS_ACTION_SIZE + 1,
                           HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

  kfree(buf);
  return ret > 0 ? 0 : ret;
}

/* ---- Name tables ---- */

/* Order MUST match firmware-side CabModel enum in CabConfiguration.h.
 * APPEND-ONLY: never reorder — the byte value is persisted in NVS. */
static const char* const cab_type_names[] = {
  "not_configured",       /*  0 = CabModel::NotConfigured       */
  "joystick",             /*  1 = CabModel::GenericJoystick     */
  "race",                 /*  2 = CabModel::GenericRace         */
  "gun",                  /*  3 = CabModel::GenericGun          */
  "crazy_taxi",           /*  4 = CabModel::CrazyTaxi           */
  "18_wheeler",           /*  5 = CabModel::EighteenWheeler     */
  "jambo_safari",         /*  6 = CabModel::JamboSafari         */
  "house_of_the_dead_3",  /*  7 = CabModel::HouseOfTheDead3     */
  "lupin_3",              /*  8 = CabModel::Lupin3              */
  "new_net_city",         /*  9 = CabModel::NewNetCity          */
  "naomi_nuc",            /* 10 = CabModel::NaomiNUC            */
  "vewlix",               /* 11 = CabModel::Vewlix              */
  "confidential_mission", /* 12 = CabModel::ConfidentialMission */
  "gun_custom",           /* 13 = CabModel::GunCustom           */
  "blast_city",           /* 14 = CabModel::JoyBlast            */
  "net_city",             /* 15 = CabModel::NetCity             */
};
#define NUM_CAB_TYPES ARRAY_SIZE(cab_type_names)

/* Runtime-only sentinel the firmware reports in cab_type while its config UI is open
 * (never a persisted CabModel). Shown as "configuring" so S05jvs-screentype gates its
 * screentype reboots until config finalises. High value = never collides with a cab. */
#define CAB_TYPE_CONFIGURING 255

// Indexed by CabFamily — extend only if CabFamily grows.
static const char* const cab_family_names[] = {
  "none",             /* 0 = CabFamily::None      */
  "joystick",         /* 1 = CabFamily::Joystick  */
  "race",             /* 2 = CabFamily::Race      */
  "gun",              /* 3 = CabFamily::Gun       */
};

// Indexed by CabScreenLock.
static const char* const cab_screen_lock_names[] = {
  "none",             /* 0 = CabScreenLock::None  */
  "khz15",            /* 1 = CabScreenLock::KHz15 */
  "khz31",            /* 2 = CabScreenLock::KHz31 */
};

// Indexed by ICrtInterface::HorizontalFrequency.
static const char* const cab_default_screentype_names[] = {
  "auto",             /* 0 = Auto         */
  "khz15",            /* 1 = KHz15        */
  "multi1525",        /* 2 = KHzMulti1525 */
  "khz31",            /* 3 = KHz31        */
  "multi1531",        /* 4 = KHzMulti1531 */
  "trifreq",          /* 5 = KHzTriFreq   */
};


static const char* const link_mode_names[] = {
  "1vs1",
  "2vs2",
};
#define NUM_LINK_MODE ARRAY_SIZE(link_mode_names)

static const char* const players_on_panel_names[] = {
  "1",
  "2",
};
#define NUM_PLAYERS_ON_PANEL ARRAY_SIZE(players_on_panel_names)

/* ---- sysfs macros ---- */

/* Read-write numeric attribute */
#define RECALBOXJVS_ATTR(_name, _offset)				\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{									\
	u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};			\
	int ret;							\
	if (!recalboxjvs_hdev)						\
		return -ENODEV;						\
	ret = recalboxjvs_get_config(recalboxjvs_hdev, config);		\
	if (ret)							\
		return ret;						\
	return sysfs_emit(buf, "%u\n", config[_offset]);		\
}									\
static ssize_t _name##_store(struct device *dev,			\
			     struct device_attribute *attr,		\
			     const char *buf, size_t count)		\
{									\
	unsigned int val;						\
	int ret;							\
	if (!recalboxjvs_hdev)						\
		return -ENODEV;						\
	if (kstrtouint(buf, 10, &val) || val > 255)			\
		return -EINVAL;						\
	ret = recalboxjvs_set_config_byte(recalboxjvs_hdev, _offset,	\
					  (u8)val);			\
	return ret ? ret : count;					\
}									\
static DEVICE_ATTR_RW(_name)

/* Read-only numeric attribute */
#define RECALBOXJVS_ATTR_RO(_name, _offset)				\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{									\
	u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};			\
	int ret;							\
	if (!recalboxjvs_hdev)						\
		return -ENODEV;						\
	ret = recalboxjvs_get_config(recalboxjvs_hdev, config);		\
	if (ret)							\
		return ret;						\
	return sysfs_emit(buf, "%u\n", config[_offset]);		\
}									\
static DEVICE_ATTR_RO(_name)

/* Read-write named attribute (shows/accepts strings or numbers) */
#define RECALBOXJVS_ATTR_NAMED(_name, _offset, _names, _count)		\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{									\
	u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};			\
	int ret;							\
	u8 val;								\
	if (!recalboxjvs_hdev)						\
		return -ENODEV;						\
	ret = recalboxjvs_get_config(recalboxjvs_hdev, config);		\
	if (ret)							\
		return ret;						\
	val = config[_offset];						\
	if (val < _count)						\
		return sysfs_emit(buf, "%s\n", _names[val]);		\
	return sysfs_emit(buf, "%u\n", val);				\
}									\
static ssize_t _name##_store(struct device *dev,			\
			     struct device_attribute *attr,		\
			     const char *buf, size_t count)		\
{									\
	unsigned int i;							\
	int ret;							\
	if (!recalboxjvs_hdev)						\
		return -ENODEV;						\
	for (i = 0; i < _count; i++) {					\
		if (sysfs_streq(buf, _names[i])) {			\
			ret = recalboxjvs_set_config_byte(		\
				recalboxjvs_hdev, _offset, i);		\
			return ret ? ret : count;			\
		}							\
	}								\
	if (kstrtouint(buf, 10, &i) || i > 255)				\
		return -EINVAL;						\
	ret = recalboxjvs_set_config_byte(recalboxjvs_hdev, _offset,	\
					  (u8)i);			\
	return ret ? ret : count;					\
}									\
static DEVICE_ATTR_RW(_name)

static ssize_t firmware_version_show(struct device* dev,
                                     struct device_attribute* attr, char* buf)
{
  struct usb_interface* intf;
  struct usb_device* udev;
  char serial[128];
  int ret;

  if (!recalboxjvs_hdev)
    return -ENODEV;

  if (!recalboxjvs_hdev->dev.parent)
    return -ENODEV;

  intf = to_usb_interface(recalboxjvs_hdev->dev.parent);
  if (!intf)
    return -ENODEV;

  udev = interface_to_usbdev(intf);
  if (!udev)
    return -ENODEV;

  if (!udev->descriptor.iSerialNumber)
    return -ENODATA;

  ret = usb_string(udev, udev->descriptor.iSerialNumber,
                   serial, sizeof(serial));
  if (ret < 0)
    return ret;

  return sysfs_emit(buf, "%s\n", serial);
}

static DEVICE_ATTR_RO(firmware_version);

/* Cache of last pushed temperature so sysfs can echo it without re-reading the
 * device. The HID driver is the authoritative writer of this value. */
static u8 last_pushed_temp;

/* Push temperature on the dedicated telemetry report. This is safe to call
 * periodically because telemetry never triggers a config notify on firmware. */
static int recalboxjvs_push_temp(struct hid_device* hdev, u8 temp)
{
  u8* buf;
  int ret;

  buf = kzalloc(RECALBOXJVS_TELEMETRY_SIZE + 1, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  buf[0] = RECALBOXJVS_TELEMETRY_REPORT_ID;
  buf[1 + TLM_RPI5_TEMP] = temp;
  /* bytes 1+1 .. 1+3 left zeroed (reserved telemetry slots) */

  ret = hid_hw_raw_request(hdev,
                           RECALBOXJVS_TELEMETRY_REPORT_ID,
                           buf,
                           RECALBOXJVS_TELEMETRY_SIZE + 1,
                           HID_FEATURE_REPORT,
                           HID_REQ_SET_REPORT);

  kfree(buf);
  if (ret > 0)
  {
    last_pushed_temp = temp;
    return 0;
  }
  return ret;
}

static int get_cpu_temp(void)
{
  struct thermal_zone_device* tz;
  int temp;

  tz = thermal_zone_get_zone_by_name("cpu-thermal");
  if (IS_ERR(tz))
    return -ENODEV;

  if (thermal_zone_get_temp(tz, &temp))
    return -EIO;

  // temp en milli°C → °C
  return temp / 1000;
}

static void recalboxjvs_temp_worker(struct work_struct* work)
{
  int temp, ret;

  if (!recalboxjvs_hdev)
  {
    hid_warn(recalboxjvs_hdev, "temp worker: no hid device\n");
    goto reschedule;
  }

  temp = get_cpu_temp();
  if (temp < 0)
  {
    hid_warn(recalboxjvs_hdev, "temp worker: failed to read temp: %d\n", temp);
    goto reschedule;
  }

  ret = recalboxjvs_push_temp(recalboxjvs_hdev, (u8)temp);
  if (ret)
    hid_warn(recalboxjvs_hdev, "temp worker: failed to push temp: %d\n", ret);
  else
    hid_dbg(recalboxjvs_hdev, "temp worker: pushed %d C\n", temp);

reschedule:
  schedule_delayed_work(&temp_work, msecs_to_jiffies(5000));
}

/* ---- Attribute definitions ---- */

/* cab_type: like RECALBOXJVS_ATTR_NAMED, but maps the CAB_TYPE_CONFIGURING sentinel
 * (firmware config UI open) to the "configuring" string. */
static ssize_t cab_type_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
	u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};
	int ret;
	u8 val;
	if (!recalboxjvs_hdev)
		return -ENODEV;
	ret = recalboxjvs_get_config(recalboxjvs_hdev, config);
	if (ret)
		return ret;
	val = config[CFG_CAB_TYPE];
	if (val == CAB_TYPE_CONFIGURING)
		return sysfs_emit(buf, "configuring\n");
	if (val < NUM_CAB_TYPES)
		return sysfs_emit(buf, "%s\n", cab_type_names[val]);
	return sysfs_emit(buf, "%u\n", val);
}
static ssize_t cab_type_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned int i;
	int ret;
	if (!recalboxjvs_hdev)
		return -ENODEV;
	for (i = 0; i < NUM_CAB_TYPES; i++) {
		if (sysfs_streq(buf, cab_type_names[i])) {
			ret = recalboxjvs_set_config_byte(recalboxjvs_hdev, CFG_CAB_TYPE, i);
			return ret ? ret : count;
		}
	}
	if (kstrtouint(buf, 10, &i) || i > 255)
		return -EINVAL;
	ret = recalboxjvs_set_config_byte(recalboxjvs_hdev, CFG_CAB_TYPE, (u8)i);
	return ret ? ret : count;
}
static DEVICE_ATTR_RW(cab_type);

// Read-only string mapper for an enum byte at a fixed offset (firmware-sourced).
#define RECALBOXJVS_ATTR_RO_NAMED(_name, _offset, _names, _fallback)	\
static ssize_t _name##_show(struct device *dev,				\
                            struct device_attribute *attr, char *buf)	\
{									\
  u8 config[RECALBOXJVS_CONFIG_SIZE] = {0};				\
  int ret;								\
  u8 val;								\
  if (!recalboxjvs_hdev)						\
    return -ENODEV;							\
  ret = recalboxjvs_get_config(recalboxjvs_hdev, config);		\
  if (ret)								\
    return ret;								\
  val = config[_offset];						\
  if (val < ARRAY_SIZE(_names))						\
    return sysfs_emit(buf, "%s\n", _names[val]);			\
  return sysfs_emit(buf, _fallback "\n");				\
}									\
static DEVICE_ATTR_RO(_name)

RECALBOXJVS_ATTR_RO_NAMED(cab_family,              CFG_CAB_FAMILY,              cab_family_names,              "none");
RECALBOXJVS_ATTR_RO_NAMED(cab_screen_lock,         CFG_CAB_SCREEN_LOCK,         cab_screen_lock_names,         "none");
RECALBOXJVS_ATTR_RO_NAMED(cab_default_screentype,  CFG_CAB_DEFAULT_SCREENTYPE,  cab_default_screentype_names,  "auto");

RECALBOXJVS_ATTR(analog_enabled,    CFG_ANALOG_ENABLED);
RECALBOXJVS_ATTR_NAMED(link_mode,        CFG_LINK_MODE,        link_mode_names,        NUM_LINK_MODE);
RECALBOXJVS_ATTR_NAMED(players_on_panel, CFG_PLAYERS_ON_PANEL, players_on_panel_names, NUM_PLAYERS_ON_PANEL);
RECALBOXJVS_ATTR(flip_gun_x,        CFG_FLIP_GUN_X);
RECALBOXJVS_ATTR(flip_gun_y,        CFG_FLIP_GUN_Y);
RECALBOXJVS_ATTR(start_quick_exit,  CFG_START_QUICK_EXIT);
RECALBOXJVS_ATTR(skip_boot_logo,    CFG_SKIP_BOOT_LOGO);
RECALBOXJVS_ATTR_RO(firmware_major, CFG_FIRMWARE_MAJOR);
RECALBOXJVS_ATTR_RO(firmware_minor, CFG_FIRMWARE_MINOR);
RECALBOXJVS_ATTR_RO(io_board_count, CFG_IO_BOARD_COUNT);
RECALBOXJVS_ATTR_RO(hw_revision,    CFG_HW_REVISION);
RECALBOXJVS_ATTR_RO(hdmi_only,      CFG_HDMI_ONLY);
RECALBOXJVS_ATTR_RO(khz15_only,     CFG_KHZ15_ONLY);

/* rpi5_temp lives on the telemetry report, not the config report.
 * show: returns the last value the kernel pushed (no HID round-trip).
 * store: forces an immediate push of the given value. */
static ssize_t rpi5_temp_show(struct device* dev,
                              struct device_attribute* attr, char* buf)
{
  return sysfs_emit(buf, "%u\n", last_pushed_temp);
}

static ssize_t rpi5_temp_store(struct device* dev,
                               struct device_attribute* attr,
                               const char* buf, size_t count)
{
  unsigned int val;
  int ret;
  if (!recalboxjvs_hdev)
    return -ENODEV;
  if (kstrtouint(buf, 10, &val) || val > 255)
    return -EINVAL;
  ret = recalboxjvs_push_temp(recalboxjvs_hdev, (u8)val);
  return ret ? ret : count;
}

static DEVICE_ATTR_RW(rpi5_temp);

/* reset_config: write any value to wipe NVS config and reboot the firmware. */
static ssize_t reset_config_store(struct device* dev,
                                  struct device_attribute* attr,
                                  const char* buf, size_t count)
{
  int ret;
  if (!recalboxjvs_hdev)
    return -ENODEV;
  ret = recalboxjvs_send_action(recalboxjvs_hdev, ACTION_RESET_CONFIG);
  return ret ? ret : count;
}

static DEVICE_ATTR_WO(reset_config);

/* flash_mode: write any value to reboot the firmware into ROM download mode. */
static ssize_t flash_mode_store(struct device* dev,
                                struct device_attribute* attr,
                                const char* buf, size_t count)
{
  int ret;
  if (!recalboxjvs_hdev)
    return -ENODEV;
  ret = recalboxjvs_send_action(recalboxjvs_hdev, ACTION_FLASH_MODE);
  return ret ? ret : count;
}

static DEVICE_ATTR_WO(flash_mode);

/* shutdown_jvs: write 1 → shutdown, write 0 → wake. Also fired automatically
 * by the reboot notifier on SYS_POWER_OFF (always shutdown). */
static ssize_t shutdown_jvs_store(struct device* dev,
                                  struct device_attribute* attr,
                                  const char* buf, size_t count)
{
  unsigned int val;
  int ret;
  if (!recalboxjvs_hdev)
    return -ENODEV;
  if (kstrtouint(buf, 10, &val))
    return -EINVAL;
  ret = recalboxjvs_send_action(recalboxjvs_hdev,
                                val ? ACTION_SHUTDOWN : ACTION_WAKE);
  return ret ? ret : count;
}

static DEVICE_ATTR_WO(shutdown_jvs);

/* Reboot notifier: when the Pi5 is powering off, signal the JVS firmware so
 * it can cut amplifiers, audio and LEDs before our USB drops. On reboot/halt
 * we stay quiet — JVS firmware keeps running, ready for the next boot. */
static int recalboxjvs_reboot_notify(struct notifier_block* nb,
                                     unsigned long code, void* unused)
{
  if (code == SYS_POWER_OFF && recalboxjvs_hdev) {
    recalboxjvs_send_action(recalboxjvs_hdev, ACTION_SHUTDOWN);
  }
  return NOTIFY_DONE;
}

static struct notifier_block recalboxjvs_reboot_nb = {
  .notifier_call = recalboxjvs_reboot_notify,
  .priority      = 0,
};

static struct attribute* recalboxjvs_attrs[] = {
  &dev_attr_cab_type.attr,
  &dev_attr_cab_family.attr,
  &dev_attr_cab_screen_lock.attr,
  &dev_attr_cab_default_screentype.attr,
  &dev_attr_analog_enabled.attr,
  &dev_attr_rpi5_temp.attr,
  &dev_attr_link_mode.attr,
  &dev_attr_players_on_panel.attr,
  &dev_attr_flip_gun_x.attr,
  &dev_attr_flip_gun_y.attr,
  &dev_attr_start_quick_exit.attr,
  &dev_attr_skip_boot_logo.attr,
  &dev_attr_reset_config.attr,
  &dev_attr_flash_mode.attr,
  &dev_attr_shutdown_jvs.attr,
  &dev_attr_firmware_major.attr,
  &dev_attr_firmware_minor.attr,
  &dev_attr_io_board_count.attr,
  &dev_attr_firmware_version.attr,
  &dev_attr_hw_revision.attr,
  &dev_attr_hdmi_only.attr,
  &dev_attr_khz15_only.attr,
  NULL,
};

static const struct attribute_group recalboxjvs_group = {
  .attrs = recalboxjvs_attrs,
};

#define RECALBOXJVS_STRING_REPORT_ID  6
#define RECALBOXJVS_STRING_SIZE       32

static int recalboxjvs_send_string(struct hid_device *hdev,
                                    u8 report_id, const char *str)
{
  u8 *buf;
  int ret;
  size_t len;

  buf = kzalloc(RECALBOXJVS_STRING_SIZE + 1, GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  buf[0] = report_id;
  len = strlen(str);
  if (len > RECALBOXJVS_STRING_SIZE)
    len = RECALBOXJVS_STRING_SIZE;
  memcpy(buf + 1, str, len);

  ret = hid_hw_raw_request(hdev, report_id, buf,
                           RECALBOXJVS_STRING_SIZE + 1,
                           HID_FEATURE_REPORT, HID_REQ_SET_REPORT);
  kfree(buf);
  return ret > 0 ? 0 : ret;
}

static char *get_recalbox_version(void)
{
  struct file *f;
  static char version[32];
  loff_t pos = 0;
  ssize_t ret;

  memset(version, 0, sizeof(version));
  f = filp_open("/recalbox/recalbox.version", O_RDONLY, 0);
  if (IS_ERR(f))
    return "unknown";

  ret = kernel_read(f, version, sizeof(version) - 1, &pos);
  filp_close(f, NULL);

  if (ret <= 0)
    return "unknown";

  /* Strip newline */
  while (ret > 0 && (version[ret - 1] == '\n' || version[ret - 1] == '\r'))
    version[--ret] = '\0';

  return version;
}

/* ---- HID driver callbacks ---- */

static int recalboxjvs_probe(struct hid_device *hdev,
                             const struct hid_device_id *id)
{
  struct device *pdev;
  int ret;

  hdev->quirks |= HID_QUIRK_MULTI_INPUT;

  ret = hid_parse(hdev);
  if (ret)
    return ret;

  ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
  if (ret)
    return ret;

  /* Only setup sysfs/worker on the first interface */
  if (recalboxjvs_hdev)
    return 0;

  recalboxjvs_hdev = hdev;

  /* Send Recalbox version to firmware */
  recalboxjvs_send_string(hdev, RECALBOXJVS_STRING_REPORT_ID,
                          get_recalbox_version());

  /* Attach sysfs to existing platform device */
  pdev = bus_find_device_by_name(&platform_bus_type, NULL, "recalboxjvs");
  if (pdev) {
    ret = sysfs_create_group(&pdev->kobj, &recalboxjvs_group);
    if (ret)
      hid_warn(hdev, "Failed to create sysfs on platform device\n");
    put_device(pdev);
  } else {
    hid_warn(hdev, "Platform device 'recalboxjvs' not found, sysfs not created\n");
  }

  INIT_DELAYED_WORK(&temp_work, recalboxjvs_temp_worker);
  schedule_delayed_work(&temp_work, msecs_to_jiffies(5000));

  ret = register_reboot_notifier(&recalboxjvs_reboot_nb);
  if (ret)
    hid_warn(hdev, "Failed to register reboot notifier: %d\n", ret);

  hid_info(hdev, "RecalboxJVS config driver loaded\n");
  return 0;
}

static void recalboxjvs_remove(struct hid_device *hdev)
{
  if (recalboxjvs_hdev != hdev) {
    hid_hw_stop(hdev);
    return;
  }

  struct device *pdev;
  unregister_reboot_notifier(&recalboxjvs_reboot_nb);
  cancel_delayed_work_sync(&temp_work);

  pdev = bus_find_device_by_name(&platform_bus_type, NULL, "recalboxjvs");
  if (pdev) {
    sysfs_remove_group(&pdev->kobj, &recalboxjvs_group);
    put_device(pdev);
  }
  recalboxjvs_hdev = NULL;
  hid_hw_stop(hdev);
}

static int recalboxjvs_input_configured(struct hid_device *hdev,
                                        struct hid_input *hidinput)
{
  struct hid_report *report = hidinput->report;
  if (!report)
    return 0;

  char *name = NULL;

  switch (report->id) {
  case 1: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gamepad #1"); break;
  case 2: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gamepad #2"); break;
  case 3: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gamepad #3"); break;
  case 4: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gamepad #4"); break;
  case 7: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gun #1"); break;
  case 8: name = kasprintf(GFP_KERNEL, "RecalboxJVS Gun #2"); break;
  }

  if (name) {
    kfree(hidinput->name);
    hidinput->name = name;
    hidinput->input->name = name;
  }

  return 0;
}

/* Forward Report 11 (video control) from ESP to rp1-dpi RGB blanking. */
static int recalboxjvs_raw_event(struct hid_device *hdev, struct hid_report *report,
                                 u8 *data, int size)
{
  if (report->id != RECALBOXJVS_VIDEO_REPORT_ID || size < 2)
    return 0;

  switch (data[1]) {
  case VIDEO_CMD_BLANK:
    rp1dpi_blank_global(true);
    break;
  case VIDEO_CMD_UNBLANK:
    rp1dpi_blank_global(false);
    break;
  default:
    break;
  }
  return 0;
}

static const struct hid_device_id recalboxjvs_devices[] = {
  {HID_USB_DEVICE(0xD191, 0x5487)},
  {}
};
MODULE_DEVICE_TABLE(hid, recalboxjvs_devices);

static struct hid_driver recalboxjvs_driver = {
  .name = "recalboxjvs",
  .id_table = recalboxjvs_devices,
  .probe = recalboxjvs_probe,
  .remove = recalboxjvs_remove,
  .input_configured = recalboxjvs_input_configured,
  .raw_event = recalboxjvs_raw_event,
};
module_hid_driver(recalboxjvs_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Digi-PCB");
MODULE_DESCRIPTION("RecalboxJVS sysfs config via HID Feature Reports");
