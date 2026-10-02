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

} gamepad_config;

static int gamepadproxy_probe(struct platform_device *pdev) {
  dev_info(&pdev->dev, "gamepadproxy_probe\n");
  return 0;
}


static void gamepadproxy_remove(struct platform_device *pdev) {
  printk(KERN_INFO "gamepadproxy_remove\n");
}


static const struct of_device_id gamepadproxy_match[] = {
  {.compatible = "recalbox-gamepad-proxy"},
  {},
};

MODULE_DEVICE_TABLE(of, gamepadproxy_match);

static struct platform_driver gamepadproxy_driver = {
  .probe  = gamepadproxy_probe,
  .remove = gamepadproxy_remove,
  .driver = {
    .name        = "recalbox-gamepad-proxy",
    .of_match_table    = gamepadproxy_match,
  },
};


static int __init gamepadproxy_init(void) {
  printk(KERN_INFO
    "recalbox-gamepad-proxy: init\n");
  /**
   * Scans all /dev/input/event*, display them and ask the user which one to
   * open.
   *
   * @return The event device file name of the device file selected. This
   * string is allocated and must be freed by the caller.
   */
  static char* scan_devices(void)
  {
    struct dirent **namelist;
    int i, ndev, devnum;
    char *filename;
    int max_device = 0;

    ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
    if (ndev <= 0)
      return NULL;

    fprintf(stderr, "Available devices:\n");

    for (i = 0; i < ndev; i++)
    {
      char fname[64];
      int fd = -1;
      char name[256] = "???";

      snprintf(fname, sizeof(fname),
         "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
      fd = open(fname, O_RDONLY);
      if (fd < 0)
        continue;
      ioctl(fd, EVIOCGNAME(sizeof(name)), name);

      fprintf(stderr, "%s:	%s\n", fname, name);
      close(fd);

      sscanf(namelist[i]->d_name, "event%d", &devnum);
      if (devnum > max_device)
        max_device = devnum;

      free(namelist[i]);
    }

    fprintf(stderr, "Select the device event number [0-%d]: ", max_device);
    scanf("%d", &devnum);

    if (devnum > max_device || devnum < 0)
      return NULL;

    asprintf(&filename, "%s/%s%d",
       DEV_INPUT_EVENT, EVENT_DEV_NAME,
       devnum);

    return filename;
  }
  return 0;
}

static void __exit gamepadproxy_exit(void) {
  printk(KERN_INFO
         "recalbox-gamepad-proxy: exiting\n");

}

module_init(gamepadproxy_init);
module_exit(gamepadproxy_exit);

MODULE_LICENSE("Copyright @digitalLumberjack");
MODULE_DESCRIPTION("RECALBOX GAMEPAD PROXY Driver");
