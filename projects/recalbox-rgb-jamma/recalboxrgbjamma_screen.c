// SPDX-License-Identifier: GPL-2.0
/*
 * RECALBOX RGB JAMMA - SSD1306 screen kernel driver
 *
 * Shared driver for RGB Jamma 1 and RGB Jamma 2 boards.
 * Exposes /dev/recalboxrgbjamma_screen as a misc device.
 * Userspace writes a 1024-byte raw framebuffer (128x64, PAGE mode encoding).
 * A workqueue flushes it to the SSD1306 via I2C, serialised with the MCP
 * polling driver that shares the same bus.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DRIVER_NAME  "recalboxrgbjamma_screen"
#define DEVICE_NAME  "recalboxrgbjamma_screen"

#define FB_PAGES     8
#define FB_COLS      128
#define FB_SIZE      (FB_PAGES * FB_COLS)   /* 1024 bytes */

/* SSD1306 I2C protocol bytes */
#define SSD1306_CMD_MODE   0x00
#define SSD1306_DATA_MODE  0x40

#define CONFIG_PATH "/boot/crt/recalbox-crt-options.cfg"
#define READ_BUF_SIZE 2048

struct screen_dev {
	struct i2c_client  *client;
	struct miscdevice   misc;
	struct work_struct  flush_work;
	struct mutex        lock;
	u8                  buf[FB_SIZE];       /* latest framebuf from userspace */
	u8                  flush_buf[FB_SIZE]; /* snapshot used by workqueue      */
	bool                rotate;
};

/* ============================================================================
 * SSD1306 low-level helpers
 * ============================================================================ */

static int ssd1306_send(struct i2c_client *cl, const u8 *payload, int len)
{
	int ret = i2c_master_send(cl, payload, len);
	return (ret == len) ? 0 : -EIO;
}

static int ssd1306_init_display(struct screen_dev *dev)
{
	struct i2c_client *cl = dev->client;

	u8 init[] = {
		SSD1306_CMD_MODE,
		0xae,        /* display off              */
		0xd5, 0x80,  /* clock div / osc freq     */
		0xa8, 63,    /* multiplex ratio (64-1)   */
		0xd3, 0x00,  /* display offset = 0       */
		0x40,        /* start line = 0           */
		0x8d, 0x14,  /* charge pump on           */
		0x20, 0x02,  /* PAGE addressing mode     */
		0xda, 0x12,  /* com pins: alt, no remap  */
		0x81, 0x7f,  /* contrast                 */
		0xd9, 0xf1,  /* pre-charge period        */
		0xdb, 0x40,  /* Vcomh deselect level     */
		0xa4,        /* resume display from RAM  */
		0xa6,        /* normal (non-inverted)    */
		0x2e,        /* stop scroll              */
	};
	int ret = ssd1306_send(cl, init, sizeof(init));
	if (ret)
		return ret;

	if (dev->rotate) {
		u8 flip[] = { SSD1306_CMD_MODE, 0xa1, 0xc8 };
		ret = ssd1306_send(cl, flip, sizeof(flip));
	} else {
		u8 norm[] = { SSD1306_CMD_MODE, 0xa0, 0xc0 };
		ret = ssd1306_send(cl, norm, sizeof(norm));
	}
	if (ret)
		return ret;

	u8 on[] = { SSD1306_CMD_MODE, 0xaf };
	return ssd1306_send(cl, on, sizeof(on));
}

static void ssd1306_clear_display(struct screen_dev *dev)
{
	struct i2c_client *cl = dev->client;
	u8 cursor[4];
	u8 data[1 + FB_COLS];
	int page;

	memset(data, 0x00, sizeof(data));
	data[0] = SSD1306_DATA_MODE;

	for (page = 0; page < FB_PAGES; page++) {
		cursor[0] = SSD1306_CMD_MODE;
		cursor[1] = 0xb0 | page;
		cursor[2] = 0x00;
		cursor[3] = 0x10;
		ssd1306_send(cl, cursor, sizeof(cursor));
		ssd1306_send(cl, data, sizeof(data));
	}
}

static void ssd1306_flush_page(struct i2c_client *cl, int page,
				const u8 *page_data)
{
	u8 cursor[4] = {
		SSD1306_CMD_MODE,
		0xb0 | page,
		0x00,
		0x10,
	};
	u8 data[1 + FB_COLS];

	data[0] = SSD1306_DATA_MODE;
	memcpy(data + 1, page_data, FB_COLS);

	ssd1306_send(cl, cursor, sizeof(cursor));
	ssd1306_send(cl, data, sizeof(data));
}

/* ============================================================================
 * Workqueue
 * ============================================================================ */

static void screen_flush_work_fn(struct work_struct *work)
{
	struct screen_dev *dev =
		container_of(work, struct screen_dev, flush_work);
	int page;

	mutex_lock(&dev->lock);
	memcpy(dev->flush_buf, dev->buf, FB_SIZE);
	mutex_unlock(&dev->lock);

	for (page = 0; page < FB_PAGES; page++)
		ssd1306_flush_page(dev->client, page,
				   dev->flush_buf + page * FB_COLS);
}

/* ============================================================================
 * Misc device file operations
 * ============================================================================ */

static ssize_t screen_write(struct file *file, const char __user *ubuf,
			    size_t count, loff_t *ppos)
{
	struct screen_dev *dev =
		container_of(file->private_data, struct screen_dev, misc);

	if (count != FB_SIZE)
		return -EINVAL;

	mutex_lock(&dev->lock);
	if (copy_from_user(dev->buf, ubuf, FB_SIZE)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	mutex_unlock(&dev->lock);

	schedule_work(&dev->flush_work);
	return count;
}

static const struct file_operations screen_fops = {
	.owner = THIS_MODULE,
	.write = screen_write,
};

/* ============================================================================
 * Config
 * ============================================================================ */

static bool read_rotate_config(void)
{
	struct file *fp;
	char *buf, *ptr, *end;
	ssize_t size;
	loff_t pos = 0;
	bool rotate = false;

	fp = filp_open(CONFIG_PATH, O_RDONLY, 0);
	if (IS_ERR(fp))
		return false;

	buf = kmalloc(READ_BUF_SIZE, GFP_KERNEL);
	if (!buf) {
		filp_close(fp, NULL);
		return false;
	}

	size = kernel_read(fp, buf, READ_BUF_SIZE - 1, &pos);
	filp_close(fp, NULL);

	if (size <= 0)
		goto out;

	buf[size] = '\0';
	ptr = buf;

	while ((end = strchr(ptr, '\n')) != NULL) {
		*end = '\0';
		if (strncmp(ptr, "options.jamma.screen.rotate = 1", 31) == 0) {
			rotate = true;
			break;
		}
		ptr = end + 1;
	}

out:
	kfree(buf);
	return rotate;
}

/* ============================================================================
 * I2C client driver
 * ============================================================================ */

static int screen_probe(struct i2c_client *client)
{
	struct screen_dev *dev;
	int ret;

	dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->client = client;
	dev->rotate = read_rotate_config();
	mutex_init(&dev->lock);
	INIT_WORK(&dev->flush_work, screen_flush_work_fn);

	ret = ssd1306_init_display(dev);
	if (ret) {
		dev_err(&client->dev, "SSD1306 init failed: %d\n", ret);
		return ret;
	}

	ssd1306_clear_display(dev);

	dev->misc.minor = MISC_DYNAMIC_MINOR;
	dev->misc.name  = DEVICE_NAME;
	dev->misc.fops  = &screen_fops;

	ret = misc_register(&dev->misc);
	if (ret) {
		dev_err(&client->dev, "misc_register failed: %d\n", ret);
		return ret;
	}

	i2c_set_clientdata(client, dev);

	dev_info(&client->dev,
		 DRIVER_NAME " ready (rotate=%d) → /dev/" DEVICE_NAME "\n",
		 dev->rotate);
	return 0;
}

static void screen_remove(struct i2c_client *client)
{
	struct screen_dev *dev = i2c_get_clientdata(client);

	cancel_work_sync(&dev->flush_work);
	ssd1306_clear_display(dev);
	misc_deregister(&dev->misc);
}

static const struct of_device_id screen_of_match[] = {
	{ .compatible = "recalbox,recalboxrgbjamma-screen" },
	{ .compatible = "recalbox,recalboxrgbjamma2-screen" },
	{ },
};
MODULE_DEVICE_TABLE(of, screen_of_match);

static struct i2c_driver screen_driver = {
	.probe  = screen_probe,
	.remove = screen_remove,
	.driver = {
		.name           = DRIVER_NAME,
		.of_match_table = screen_of_match,
	},
};

module_i2c_driver(screen_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Recalbox Team");
MODULE_DESCRIPTION("RECALBOX RGB JAMMA – SSD1306 screen driver (rrgbj1 + rrgbj2)");
