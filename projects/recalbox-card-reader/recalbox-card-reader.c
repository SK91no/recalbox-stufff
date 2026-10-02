#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/dev_printk.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include "./firmware.h"
#include "./firmware-proto.h"
#include "./firmware1.1.h"

static struct proc_dir_entry * proc_file;
static struct i2c_client *card_client;
static bool updating = false;
static struct mutex process_mutex;


enum DataType {
    Byte,
    Word,
};

// Command Lists
#define VERSION_COMMAND             (0x01)
#define PLUGGED_COMMAND             (0x10)
#define SEAZON_COMMAND              (0x11)
#define ID_COMMAND                  (0x12)
#define REVERSED_COMMAND            (0x13)
#define SPECIFIC_COMMAND            (0x14)
#define REBOOT_BOOTMODE             (0x15)
#define UPGRADE_COMMAND             (0x16)
#define UPGRADE_DONE                (0x17)
#define FIRMWARE_VERSION_COMMAND    (0x18)
#define REBOOT_ON_MISSING_COMMANDS  (0x19)
#define OPENCARDPOWER_COMMAND       (0x20)
#define RESET_ASSOCIATION           (0x21)
#define SAVE_GAME_FOR_CARD          (0x22)
#define GET_GAME_FOR_CARD           (0x23)
#define GET_GAME_FOR_CARD_LEN       (0x24)


static int cardreader_read(int command, enum DataType dtype)
{
  int res;
  mutex_lock(&process_mutex);
  for(int loop = 3; --loop >= 0;){
    if (dtype == Byte)
      res = i2c_smbus_read_byte_data(card_client, command);
    else
      res = i2c_smbus_read_word_data(card_client, command);
    if(res < 0) {
      printk(KERN_INFO "recalbox-card-reader: cardreader_read: Unable to communicate with card reader, retrying: %d\n", loop);
      usleep_range(20000, 40000);
    } else {
      break;
    }
  }
  usleep_range(20000, 40000);
  mutex_unlock(&process_mutex);
  return res;
}

static ssize_t cardreader_read_and_show(char *buf, char * name, int command, enum DataType dtype, bool booleanize)
{
  if(updating)
    return sysfs_emit(buf, "updating");
  int res = cardreader_read(command, dtype);
  if(booleanize)
    return sysfs_emit(buf, "%s", res <= 0 ? "false" : "true");
  return sysfs_emit(buf, "%d", res);
}

static ssize_t cardreader_available_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_available_show", VERSION_COMMAND, Byte, true);
}

static ssize_t cardreader_plugged_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_plugged_show", PLUGGED_COMMAND, Byte, true);
}


static ssize_t cardreader_seazon_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_seazon_show", SEAZON_COMMAND, Byte, false);
}

static ssize_t cardreader_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_id_show", ID_COMMAND, Word, false);
}

static ssize_t cardreader_reversed_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_reversed_show", REVERSED_COMMAND, Byte, false);
}

static ssize_t cardreader_specific_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_specific_show", SPECIFIC_COMMAND, Byte, false);
}

static ssize_t cardreader_version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return cardreader_read_and_show(buf, "cardreader_version_show", FIRMWARE_VERSION_COMMAND, Byte, false);
}

static ssize_t cardreader_module_version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sysfs_emit(buf, "%d", recalbox_card_reader_bin_version);
}

static ssize_t cardreader_reset_store(struct  kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  unsigned char cmd = RESET_ASSOCIATION;
  mutex_lock(&process_mutex);
  int err = i2c_master_send(card_client, &cmd, 1);
  usleep_range(20000, 40000);
  mutex_unlock(&process_mutex);
  if(err < 0) {
    dev_err(&card_client->dev, "cardreader_reset_store: Unable to reset association on card reader.\n");
  }
  return count;
}


static ssize_t cardreader_uuid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  int seazon = cardreader_read(SEAZON_COMMAND, Byte);
  int id = cardreader_read(ID_COMMAND, Word);
  if(id < 0 || seazon < 0) {
    printk(KERN_INFO "recalbox-card-reader: cardreader_uuid_show: Unable to communicate with card reader, error %d %d\n", id, seazon);
    return sysfs_emit(buf, "%d", -1);
  }
  id = (id & 0xFFF) | seazon << 16;
  return sysfs_emit(buf, "%d", id);
}

static ssize_t cardreader_association_store(struct  kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  if(count > 4095){
    dev_warn(&card_client->dev, "cardreader_association_store: Buffer too large.\n");
    return count;
  }
  char *message = kmalloc(count+1, GFP_KERNEL);
  message[0] = SAVE_GAME_FOR_CARD;
  memcpy(message+1, buf, count);
  dev_info(&card_client->dev, "cardreader_association_store: sending the string %s to card reader with size %d and command %x.\n", buf, count, message[0]);

  mutex_lock(&process_mutex);
  int err = i2c_master_send(card_client, message, count+1);
  usleep_range(20000, 40000);
  mutex_unlock(&process_mutex);
  if(err < 0) {
    kfree(message);
    dev_err(&card_client->dev, "cardreader_association_store: Unable to associate game %s to card reader. err: %d\n", buf, err);
    return -1;
  }
  kfree(message);
  return count;
}

static ssize_t cardreader_association_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  char size[2] = {};
  char command = GET_GAME_FOR_CARD_LEN;
  mutex_lock(&process_mutex);
  int err = i2c_master_send(card_client, &command , 1);
  usleep_range(20000, 40000);

  if(err < 0){
    dev_err(&card_client->dev, "cardreader_association_show: Could not request association\n");
    mutex_unlock(&process_mutex);
    return -1;
  }
  // Read len
  err = i2c_master_recv(card_client, size, 2);
  usleep_range(20000, 40000);
  if(err != 2){
    dev_err(&card_client->dev, "cardreader_association_show: Could not read association string len\n");
    mutex_unlock(&process_mutex);
    return -1;
  }
  unsigned int toRead = (size[0] << 8) | size[1];
  //dev_info(&card_client->dev, "Len to receive = %u bytes\n", toRead);
  if(toRead > 4096){
    mutex_unlock(&process_mutex);
    dev_err(&card_client->dev, "cardreader_association_show: read len too long : %d\n", toRead);
    return -1;
  }

  // Get string
  command = GET_GAME_FOR_CARD;
  char received[4096] = {};
  err = i2c_master_send(card_client, &command , 1);
  usleep_range(20000, 40000);
  err = i2c_master_recv(card_client, received, toRead);
  usleep_range(20000, 40000);
  mutex_unlock(&process_mutex);

  if(err != toRead){
    dev_err(&card_client->dev, "cardreader_association_show: Could not read association string\n");
    return -1;
  }
  //dev_info(&card_client->dev, "Received %d bytes for string %s\n", err, received);
  return sysfs_emit(buf, "%s", received);
}

#define CR_FILE_COUNT 11
static const struct kobj_attribute sysfs_files[CR_FILE_COUNT] = {
  __ATTR(available, 0444, cardreader_available_show, NULL),
  __ATTR(plugged, 0444, cardreader_plugged_show, NULL),
  __ATTR(season, 0444, cardreader_seazon_show, NULL),
  __ATTR(id, 0444, cardreader_id_show, NULL),
  __ATTR(reversed, 0444, cardreader_reversed_show, NULL),
  __ATTR(specific, 0444, cardreader_specific_show, NULL),
  __ATTR(uuid, 0444, cardreader_uuid_show, NULL),
  __ATTR(association, 0644, cardreader_association_show, cardreader_association_store),
  __ATTR(reset_card, 0644, NULL, cardreader_reset_store),
  __ATTR(firmware_version, 0444, cardreader_version_show, NULL),
  __ATTR(module_version, 0444, cardreader_module_version_show, NULL),
};

static struct kobject *cardreader_kobj;

#define CHUNK_SIZE 4096
static int cardreader_update_firmware(void *idx) {
  unsigned char buf[CHUNK_SIZE + 2] = {};
  int err = 0;

  mutex_lock(&process_mutex);
  int hw_version = i2c_smbus_read_byte_data(card_client, VERSION_COMMAND);
  mutex_unlock(&process_mutex);

  if(hw_version < 0) {
    dev_warn(&card_client->dev, "cardreader_update_firmware: Unable to get hardware version on card reader reader.\n");
    return 0;
  }
  unsigned int firmware_len = hw_version == 1 ? recalbox_card_reader_proto_bin_len : (hw_version == 0xf ? recalbox_card_reader_1_1_bin_len : recalbox_card_reader_bin_len);
  unsigned char * firmware_bin = hw_version == 1 ? recalbox_card_reader_proto_bin : (hw_version == 0xf ? recalbox_card_reader_1_1_bin : recalbox_card_reader_bin);

  int chunks = firmware_len / CHUNK_SIZE + 1;
  dev_info(&card_client->dev, "cardreader_update_firmware: Will send new firmware of %d bytes over %d chunks.\n", firmware_len, chunks );
  for(int chunk = 0; chunk < chunks; chunk++){
    int to_copy = firmware_len - chunk * CHUNK_SIZE > CHUNK_SIZE ? CHUNK_SIZE : firmware_len - chunk * CHUNK_SIZE;
    buf[0] = UPGRADE_COMMAND;
    buf[1] = chunk;
    memcpy(buf +2, firmware_bin + chunk * CHUNK_SIZE, CHUNK_SIZE);
    mutex_lock(&process_mutex);
    err = i2c_master_send(card_client, buf, to_copy+2);
    mutex_unlock(&process_mutex);
    if(err < 0) {
      dev_err(&card_client->dev, "cardreader_update_firmware: Unable to upgrade card reader.\n");
      break;
    }
    usleep_range(50000, 80000);
  }
  dev_err(&card_client->dev, "cardreader_update_firmware: New firmware sent, sending UPGRADE_DONE command.\n");
  buf[0] = UPGRADE_DONE;
  mutex_lock(&process_mutex);
  err = i2c_master_send(card_client, buf, 1);
  mutex_unlock(&process_mutex);

  if(err < 0) {
    dev_err(&card_client->dev, "cardreader_update_firmware: Unable to finish upgrade on card reader.\n");
  }
  updating = false;
  return 0;
}

struct task_struct *update_t;


static int cardreader_probe(struct i2c_client *client) {
  dev_info(&client->dev, "cardreader_probe: Welcome to Recalbox card Reader.\n");
  int err = 0;
  mutex_init(&process_mutex);

  cardreader_kobj = kobject_create_and_add("recalbox-card-reader", kernel_kobj);

  if(!cardreader_kobj) {
    dev_err(&client->dev, "cardreader_probe: unable to create /sys/kernel/recalbox-card-reader\n");
  } else {
    for(int file = 0; file < CR_FILE_COUNT; file++){
      if(sysfs_create_file(cardreader_kobj, &sysfs_files[file].attr)) {
        dev_err(&client->dev, "unable to create /sys/kernel/recalbox-card-reader/%s\n", sysfs_files[file].attr);
        kobject_put(cardreader_kobj);
        return 1;
      }
    }
  }
  dev_info(&client->dev, "cardreader_probe: Card Reader is ready.\n");

  card_client = client;

  int hw_version = i2c_smbus_read_byte_data(card_client, VERSION_COMMAND);
  if(hw_version < 0) {
    dev_info(&client->dev, "cardreader_probe: Unable to get hardware version on card reader reader.\n");
    return 0;
  }
  dev_info(&client->dev, "cardreader_probe: Card Reader hardware version: %d%s\n", hw_version, hw_version == 1 ? " - Proto" : "");

  int firmware_to_check = hw_version == 1 ? recalbox_card_reader_proto_bin_version : (hw_version == 0xf ? recalbox_card_reader_1_1_bin_version : recalbox_card_reader_bin_version);

  int firmare_version = i2c_smbus_read_byte_data(card_client, FIRMWARE_VERSION_COMMAND);
  if(firmare_version <= 0) {
    dev_info(&client->dev, "cardreader_probe: Unable to get firmware version on card reader reader.\n");
    return 0;
  }

  int idx = 0;
  dev_info(&client->dev, "cardreader_probe: card reader version: %d, module version: %u\n", firmare_version, firmware_to_check);
  if(firmare_version < firmware_to_check){
    updating = true;
    update_t = kthread_create(cardreader_update_firmware, &idx, "kthread_cardreader_update_firmware");
    if (update_t != NULL) {
      wake_up_process(update_t);
      dev_info(&client->dev, "cardreader_probe: starting update thread!\n");
    } else {
      dev_warn(&client->dev, "cardreader_probe: unable to start update thread!\n");
    }
  } else {
    dev_info(&client->dev, "cardreader_probe: switching card reader mode to reboot on broken link\n");
    i2c_smbus_read_byte_data(card_client, REBOOT_ON_MISSING_COMMANDS);
  }

  return 0;
}

static void cardreader_remove(struct i2c_client *client) {
  printk(KERN_INFO "recalbox-card-reader: Removing card Reader\n");
  if(cardreader_kobj){
    //sysfs_remove_file(cardreader_kobj, &plugged_attr.attr);
    kobject_put(cardreader_kobj);
  }
}

static const struct of_device_id cardreader_match[] = {
  {.compatible = "recalbox,card-reader"},
  {},
};

MODULE_DEVICE_TABLE(of, cardreader_match);

static const struct i2c_device_id cardreader_id[] = {
  {"recalbox-card-reader", 0},
  {}};

MODULE_DEVICE_TABLE(i2c, cardreader_id);

static struct i2c_driver cardreader_driver = {
  .probe  = cardreader_probe,
  .remove = cardreader_remove,
  .id_table = cardreader_id,
  .driver = {
    .name        = "recalbox-card-reader",
    .of_match_table    = cardreader_match,
  },
};

module_i2c_driver(cardreader_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RECALBOX CARD READER Driver");
MODULE_AUTHOR("digitalLumberjack@recalbox.com");
