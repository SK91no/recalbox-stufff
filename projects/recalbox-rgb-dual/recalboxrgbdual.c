/*
 * Copyright (C) 2018 Hugh Cole-Baker
 *
 * Hugh Cole-Baker <sigmaris@gmail.com>
 * cpasjuste
 * digitalLumberjack <digitalLumberjack@recalbox.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/reboot.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include <uapi/linux/media-bus-format.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>

// ~ 20 timings line + comments
#define READ_SIZE_MAX 2048
#define LINE_SIZE_MAX 256

static char read_buf[READ_SIZE_MAX];
static const char *timings_path = "/boot/crt/timings";
static const char *desktop_mode_path = "/tmp/crtdesktopres";
static const char *config_path = "/boot/crt/recalbox-crt-options.cfg";

// Global config
static struct rrgbdualconfiguration {
  int voffset;
  int hoffset;
};


struct dpidac {
  struct drm_bridge bridge;
  struct drm_connector connector;
};

// Gpio status
static struct gpiodesc {
  struct gpio_desc *gpio;
  int gpio_state;
};

#include "mode_logic.h"


static struct sconfig {
  struct gpiodesc gpioIn50Hz;
  struct gpiodesc gpioIn31kHz;
  struct gpiodesc gpioInComposite;
  struct gpiodesc gpioOutAudioEnable;
  struct gpiodesc gpioOutVideoFilterBypass;
  struct gpiodesc gpioOutDacEnable;
  struct gpiodesc gpioOutRGBBlanking;
  struct gpiodesc gpioOutWideScreen;
  struct gpiodesc gpioOutComposite;
  struct gpiodesc gpioOutScartDisable;
  struct gpiodesc gpioOutJackComp;
  struct gpiodesc gpioOutRPI5CSync;
  struct gpiodesc gpioOutPioSync;

  enum HatReference current_hat;
  enum ScreenType screentype;
  struct task_struct *config_thread;
  int desktop_res;
  bool widescreen;
  bool avoid_interlaced;
  bool force_composite;
  bool unplugged;
} config;

enum ModeIds {
  p320x240 = 0,
  p1920x240,
  p1920x224,
  p384x288,
  p1920x288,
  p1920x240at120,
  i768x576,
  i640x480,
  p640x480,
  p320x240jamma,
  p1920x480,
  p1920x384,
  p496x384,
  ModeCount,
};

static const char* ModeNames[ModeCount+1] = {
    "p320x240",
    "p1920x240",
    "p1920x224",
    "p384x288",
    "p1920x288",
    "p1920x240at120",
    "i768x576",
    "i640x480",
    "p640x480",
    "p320x240jamma",
    "p1920x480",
    "p1920x384",
    "p496x384",
    "ModeCount",
};

int modeNameToModeId(char * modeName){
  for(int i = 0; i < ModeCount; i++){
    if(strcmp(modeName, ModeNames[i]) == 0){
      return i;
    }
  }
  return -1;
}

static struct mode_offsets {
  int voffset;
  int hoffset;
};
static struct mode_offsets modeconfigs[ModeCount] = {
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
    {.voffset = 0, .hoffset = 0},
};
// Same modes are located in the frontend to manage the min / max porsh
static struct videomode modes[ModeCount] = {

    // 240p@60 : 320 1 4 30 46 240 1 4 5 14 0 0 0 60 0 6400000 1
    {
        .pixelclock = 6400000,
        .hactive = 320,
        .hfront_porch = 4,
        .hsync_len = 30,
        .hback_porch = 46,
        .vactive = 240,
        .vfront_porch = 4,
        .vsync_len = 5,
        .vback_porch = 14,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x240p@60 : 1920 1 80 184 312 240 1 1 3 16 0 0 0 60 0 38937600 1
    {
        .pixelclock = 38937600,
        .hactive = 1920,
        .hfront_porch = 80,
        .hsync_len = 184,
        .hback_porch = 312,
        .vactive = 240,
        .vfront_porch = 1,
        .vsync_len = 3,
        .vback_porch = 16,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x224p@60 : 1920 1 80 184 312 224 1 10 3 24 0 0 0 60 0 39087360 1
    {
        .pixelclock = 39087360,
        .hactive = 1920,
        .hfront_porch = 80,
        .hsync_len = 184,
        .hback_porch = 312,
        .vactive = 224,
        .vfront_porch = 10,
        .vsync_len = 3,
        .vback_porch = 24,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 288p@50 : 384 1 16 32 40 288 1 3 2 19 0 0 0 50 0 7363200
    {
        .pixelclock = 7363200,
        .hactive = 384,
        .hfront_porch = 16,
        .hsync_len = 32,
        .hback_porch = 40,
        .vactive = 288,
        .vfront_porch = 3,
        .vsync_len = 2,
        .vback_porch = 19,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x288p@50 : 1920 1 80 184 312 288 1 4 3 18 0 0 0 50 0 39062400 1
    {
        .pixelclock = 39062400,
        .hactive = 1920,
        .hfront_porch = 80,
        .hsync_len = 184,
        .hback_porch = 312,
        .vactive = 288,
        .vfront_porch = 4,
        .vsync_len = 3,
        .vback_porch = 18,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x240p@120 : 1920 1 48 208 256 240 1 4 3 15 0 0 0 120 0 76462080 1
    {
        .pixelclock = 76462080,
        .hactive = 1920,
        .hfront_porch = 48,
        .hsync_len = 208,
        .hback_porch = 256,
        .vactive = 240,
        .vfront_porch = 4,
        .vsync_len = 3,
        .vback_porch = 15,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },

    // 576i@50 : 768 1 24 72 88 576 1 6 5 38 0 0 0 50 1 14875000 1
    {
      .pixelclock = 14875000,
      .hactive = 768,
      .hfront_porch = 24,
      .hsync_len = 72,
      .hback_porch = 88,
      .vactive = 576,
      .vfront_porch = 6,
      .vsync_len = 5,
      .vback_porch = 38,
      .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_INTERLACED
    },

    // 480i@60 : 640 1 24 64 104 480 1 3 6 34 0 0 0 60 1 13054080 1
   {
      .pixelclock = 13054080,
      .hactive = 640,
      .hfront_porch = 24,
      .hsync_len = 64,
      .hback_porch = 104,
      .vactive = 480,
      .vfront_porch = 3,
      .vsync_len = 6,
      .vback_porch = 34,
      .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_INTERLACED
    },
    // 480p@60 : 640 1 24 96 48 480 1 11 2 32 0 0 0 60 0 25452000 1
    {
      .pixelclock = 25452000,
      .hactive = 640,
      .hfront_porch = 24,
      .hsync_len = 96,
      .hback_porch = 48,
      .vactive = 480,
      .vfront_porch = 11,
      .vsync_len = 2,
      .vback_porch = 32,
      .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    /* GunCon 2-friendly 480p@60 — kept here for reference. Switchres
     * picks this exact timing when the gun is connected on a 31 kHz
     * screen (see libretroLightGuns.py + switchres.ini), so RetroArch
     * games run on it and the photodiode locks X cleanly. The default
     * mode above (25.452 MHz / htotal=808) makes the GunCon 2 wrap
     * mid-screen, but is needed for standalones that don't drive
     * switchres.
     * {
     *   .pixelclock   = 26176500,    // 26.176 MHz
     *   .hactive      = 640,
     *   .hfront_porch = 42,
     *   .hsync_len    = 123,
     *   .hback_porch  = 26,          // htotal = 831
     *   .vactive      = 480,
     *   .vfront_porch = 15,
     *   .vsync_len    = 7,
     *   .vback_porch  = 23,          // vtotal = 525
     *   .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
     * },
     */

    //240p@60 jamma : 320 1 16 32 56 240 1 2 3 16 0 0 0 60 0 6639840 1
    {
        .pixelclock = 6639840,
        .hactive = 320,
        .hfront_porch = 20,
        .hsync_len = 32,
        .hback_porch = 52,
        .vactive = 240,
        .vfront_porch = 2,
        .vsync_len = 3,
        .vback_porch = 16,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x480p@60 : 1920 1 48 208 256 480 1 15 3 26 0 0 0 60 0 76462080 1
    {
        .pixelclock = 76462080,
        .hactive = 1920,
        .hfront_porch = 48,
        .hsync_len = 208,
        .hback_porch = 256,
        .vactive = 480,
        .vfront_porch = 15,
        .vsync_len = 3,
        .vback_porch = 26,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x384p@60 : 1920 1 48 240 192 384 1 2 5 25 0 0 0 60 0 59904000 1
    {
        .pixelclock = 59904000,
        .hactive = 1920,
        .hfront_porch = 48,
        .hsync_len = 240,
        .hback_porch = 192,
        .vactive = 384,
        .vfront_porch = 2,
        .vsync_len = 5,
        .vback_porch = 25,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 496x384p@60 : 496 1 12 62 49 384 1 2 5 25 0 0 0 60 0 15450240 1
    {
        .pixelclock = 15450240,
        .hactive = 496,
        .hfront_porch = 12,
        .hsync_len = 62,
        .hback_porch = 49,
        .vactive = 384,
        .vfront_porch = 2,
        .vsync_len = 5,
        .vback_porch = 25,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
};


static void dpidac_offset_and_validate(struct videomode *vm, int hoffset, int voffset) {

  // Horizontal offseting
  hoffset *= vm->hactive / 320;
  if ((int) vm->hfront_porch - hoffset < 1) {
    // if the porch is too high
    hoffset = vm->hfront_porch;
  }
  if ((int) vm->hback_porch + hoffset < 0) {
    // Porch is too low
    hoffset = - vm->hback_porch;
  }
  vm->hfront_porch -= hoffset;
  vm->hback_porch += hoffset;

  // Vertical offseting
  int min_voffset = 1;
  if (vm->flags & DISPLAY_FLAGS_INTERLACED) {
    // Interlaced modes won't accept a vertical porch < 2
    min_voffset = 2;
  }
  if ((int) vm->vfront_porch - voffset < min_voffset) {
    // if the porch is too high
    voffset = vm->vfront_porch - min_voffset;
  }
  if ((int) vm->vback_porch + voffset < 0) {
    // Porch is too low
    voffset = - vm->vback_porch;
  }

  vm->vfront_porch -= voffset;
  vm->vback_porch += voffset;
  printk(KERN_INFO "[RECALBOXRGBDUAL]: modified mode %dx%d - V:%d %d %d - H:%d %d %d\n",
         vm->hactive,
         vm->vactive,
         vm->vfront_porch,
         vm->vsync_len,
         vm->vback_porch,
         vm->hfront_porch,
         vm->hsync_len,
         vm->hback_porch);

}

static struct drm_display_mode *dpidac_display_mode_from_timings(struct drm_connector *connector, const char *line) {
  int ret, hsync, vsync, interlace, ratio;
  struct drm_display_mode *mode = NULL;
  struct videomode vm;

  if (line != NULL) {
    memset(&vm, 0, sizeof(vm));
    ret = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %*s %*s %*s %*s %d %ld %d",
                 &vm.hactive, &hsync, &vm.hfront_porch, &vm.hsync_len, &vm.hback_porch,
                 &vm.vactive, &vsync, &vm.vfront_porch, &vm.vsync_len, &vm.vback_porch,
                 &interlace, &vm.pixelclock, &ratio);
    if (ret != 13) {
      printk(KERN_WARNING "[RECALBOXRGBDUAL]: malformed mode requested, skipping (%s)\n", line);
      return NULL;
    }

    // setup flags
    vm.flags = interlace ? DISPLAY_FLAGS_INTERLACED : 0;
    vm.flags |= hsync ? DISPLAY_FLAGS_HSYNC_LOW : DISPLAY_FLAGS_HSYNC_HIGH;
    vm.flags |= vsync ? DISPLAY_FLAGS_VSYNC_LOW : DISPLAY_FLAGS_VSYNC_HIGH;

    // create/init display mode, convert from video mode
    mode = drm_mode_create(connector->dev);
    if (mode == NULL) {
      printk(KERN_WARNING "[RECALBOXRGBDUAL]: drm_mode_create failed, skipping (%s)\n", line);
      return NULL;
    }

    dpidac_offset_and_validate(&vm, 0, 0);
    drm_display_mode_from_videomode(&vm, mode);

    return mode;
  }

  return NULL;
}

int load_forced_desktop(int * width, int * height, bool * interlaced) {
  struct file *fp = NULL;
  ssize_t read_size = 0;
  size_t cursor = 0;
  char line[16];
  size_t line_start = 0;
  size_t line_len = 0;
  int scanret = 0;

  int readWidth = 0, readHeight = 0;
  char readInterlaced;
  fp = filp_open(desktop_mode_path, O_RDONLY, 0);
  if (IS_ERR(fp) || !fp) {
    return 0;
  }

  read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
  if (read_size <= 0) {
    filp_close(fp, NULL);
    return 0;
  }
  filp_close(fp, NULL);
  for (cursor = 0; cursor < read_size; cursor++) {
    line[cursor - line_start] = read_buf[cursor];
    line_len++;
    if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0' || cursor == read_size-1) {
      if (line_len > 1 && line[0] != '#') {
        if(line[line_len - 1] == '\n')
          line[line_len - 1] = '\0';
        scanret = sscanf(line, "%dx%d%c", &readWidth, &readHeight, &readInterlaced);
        if (scanret >= 2) {
          printk(KERN_INFO "[RECALBOXRGBDUAL]: found special mode %s\n", line);
          *width = readWidth;
          *height = readHeight;
          *interlaced = scanret == 3 && readInterlaced == 'i';
          return 1;
        }
      }
      line_start += line_len;
      line_len = 0;
      memset(line, 0, 16);
    }
  }
  return 0;
}

int dpidac_load_timings(struct drm_connector *connector) {
  struct file *fp = NULL;
  ssize_t read_size = 0;
  size_t cursor = 0;
  char line[LINE_SIZE_MAX];
  size_t line_start = 0;
  size_t line_len = 0;
  struct drm_display_mode *mode = NULL;
  int mode_count = 0;

  fp = filp_open(timings_path, O_RDONLY, 0);
  if (IS_ERR(fp) || !fp) {
    printk(KERN_WARNING "[RECALBOXRGBDUAL]: timings file not found, skipping custom modes loading\n");
    return 0;
  }

  read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
  if (read_size <= 0) {
    filp_close(fp, NULL);
    printk(KERN_WARNING "[RECALBOXRGBDUAL]: empty timings file found, skipping custom modes loading\n");
    return 0;
  }
  filp_close(fp, NULL);

  for (cursor = 0; cursor < read_size; cursor++) {
    line[cursor - line_start] = read_buf[cursor];
    line_len++;
    if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0') {
      if (line_len > 32 && line[0] != '#') {
        line[line_len - 1] = '\0';
        if ((mode = dpidac_display_mode_from_timings(connector, line)) != NULL) {
          mode->type = mode_count ? DRM_MODE_TYPE_DRIVER : DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
          printk(KERN_INFO "[RECALBOXRGBDUAL]: \t" DRM_MODE_FMT, DRM_MODE_ARG(mode));

          drm_mode_probed_add(connector, mode);
          mode_count++;
        }
      }
      line_start += line_len;
      line_len = 0;
      memset(line, 0, 128);
    }
  }

  return mode_count;
}

static bool isRGBJAMMA(void){ return ml_isRGBJAMMA(config.current_hat); }
static bool isJVS(void){ return ml_isJVS(config.current_hat); }

static bool currentHatIsRecalbox(void){
  return isRGBJAMMA() || isJVS() || config.current_hat == RecalboxRGBDual || config.current_hat == RecalboxRGBDual2;
}



static int dpidac_load_config(const char *configfile) {
  struct file *fp = NULL;
  ssize_t read_size = 0;
  size_t cursor = 0;
  char line[LINE_SIZE_MAX];
  char optionname[LINE_SIZE_MAX];
  char optionbuffer[LINE_SIZE_MAX];
  int optionvalue = 0;
  size_t line_start = 0;
  size_t line_len = 0;
  int scanret = 0;
  int modeId = 0;

  fp = filp_open(config_path, O_RDONLY, 0);
  if (IS_ERR(fp) || !fp) {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: config file not found, skipping configuration loading\n");
    return 0;
  }

  read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
  if (read_size <= 0) {
    filp_close(fp, NULL);
    printk(KERN_INFO "[RECALBOXRGBDUAL]: empty config file found, skipping configuration loading\n");
    return 0;
  }
  filp_close(fp, NULL);
  for (cursor = 0; cursor < read_size; cursor++) {
    line[cursor - line_start] = read_buf[cursor];
    line_len++;
    if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0' || cursor == read_size-1) {
      if (line_len > 1 && line[0] != '#') {
        if(line[line_len - 1] == '\n')
          line[line_len - 1] = '\0';
        scanret = sscanf(line, "%s = %d", &optionname, &optionvalue);
        if (scanret == 2) {
          if(currentHatIsRecalbox()){
            if (strcmp(optionname, "options.screen.type") == 0 && optionvalue != config.screentype) {
              printk(KERN_INFO "[RECALBOXRGBDUAL]: screen: setting %s to %d\n", optionname, optionvalue);
              config.screentype = optionvalue;
            }
            if (strcmp(optionname, "options.es.resolution") == 0 && optionvalue != config.desktop_res) {
                printk(KERN_INFO "[RECALBOXRGBDUAL]: screen: setting desktop_res to %d\n", optionvalue);
                config.desktop_res = optionvalue;
            }
            if (strcmp(optionname, "options.video.widescreen") == 0 && (optionvalue == 1) != config.widescreen) {
                printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 screen: setting widescreen to %d\n", optionvalue);
                config.widescreen = optionvalue == 1;
                if(config.gpioOutWideScreen.gpio) {
                  gpiod_set_value_cansleep(config.gpioOutWideScreen.gpio, config.widescreen);
                }
            }
            if (strcmp(optionname, "options.dual2.forcecomposite") == 0 && (optionvalue == 1) != config.force_composite) {
                printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 screen: setting force_composite to %d\n", optionvalue);
                config.force_composite = optionvalue == 1;
            }
          }
          for(modeId = 0; modeId < ModeCount; modeId++){
            sprintf(optionbuffer, "mode.offset.%s.verticaloffset", ModeNames[modeId]);
            if (strcmp(optionname, optionbuffer) == 0) {
              //printk(KERN_INFO "[RECALBOXRGBDUAL]: setting %s to %d\n", optionbuffer, optionvalue);
              modeconfigs[modeId].voffset = optionvalue;
            }
            sprintf(optionbuffer, "mode.offset.%s.horizontaloffset", ModeNames[modeId]);
            if (strcmp(optionname, optionbuffer) == 0) {
              //printk(KERN_INFO "[RECALBOXRGBDUAL]: setting %s to %d\n", optionbuffer, optionvalue);
              modeconfigs[modeId].hoffset = optionvalue;
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


static inline struct dpidac *drm_bridge_to_dpidac(struct drm_bridge *bridge) {
  return container_of(bridge, struct dpidac, bridge);
}

static inline struct dpidac *drm_connector_to_dpidac(struct drm_connector *connector) {
  return container_of(connector, struct dpidac, connector);
}


static void dpidac_apply_module_mode(struct drm_connector *connector, int modeId, bool preferred) {
  struct drm_device *dev = connector->dev;
  struct drm_display_mode *mode = drm_mode_create(dev);
  struct videomode vmcopy;
  struct videomode *vm = &modes[modeId];
  vmcopy.vback_porch = vm->vback_porch;
  vmcopy.vfront_porch = vm->vfront_porch;
  vmcopy.hback_porch = vm->hback_porch;
  vmcopy.hfront_porch = vm->hfront_porch;
  vmcopy.flags = vm->flags;
  vmcopy.hactive = vm->hactive;
  vmcopy.hsync_len = vm->hsync_len;
  vmcopy.pixelclock = vm->pixelclock;
  vmcopy.vactive = vm->vactive;
  vmcopy.vsync_len = vm->vsync_len;

  dpidac_offset_and_validate(&vmcopy, modeconfigs[modeId].hoffset, modeconfigs[modeId].voffset);
  drm_display_mode_from_videomode(&vmcopy, mode);
  mode->type = DRM_MODE_TYPE_DRIVER;
  if (preferred)
    mode->type |= DRM_MODE_TYPE_PREFERRED;

/*  if(config.current_hat == RecalboxRGBJAMMA){
    mode->flags |= (DRM_MODE_FLAG_CSYNC | DRM_MODE_FLAG_NCSYNC);
  }*/
  drm_mode_set_name(mode);
  drm_mode_probed_add(connector, mode);
}

static bool forced31kHzOnly(void){
  return ml_forced31kHzOnly(config.screentype, config.current_hat, config.gpioIn31kHz.gpio_state);
}

static bool forced50Hz(void){
	return config.gpioIn50Hz.gpio_state == 0;
}

static bool supports31kHz(void){
  return ml_supports31kHz(config.screentype, config.current_hat, config.gpioIn31kHz.gpio_state);
}

static bool supports25kHz(void){
  return ml_supports25kHz(config.screentype, config.current_hat, config.gpioIn31kHz.gpio_state);
}

static bool supports31kHz120Hz(void){
  return ml_supports31kHz120Hz(config.current_hat);
}

static bool supports50Hz(void){
  return ml_supports50Hz(config.current_hat);
}


static bool is_compatible_with_current_config(enum ModeIds mode_id){
  switch(mode_id){
    case p320x240:
    case p1920x240:
    case p1920x224:
    case p320x240jamma:
      return !forced31kHzOnly() && !forced50Hz(); // Gpio default = 1 on jamma
    case p384x288:
    case p1920x288:
      return !forced31kHzOnly() && supports50Hz();
    case p1920x240at120:
      return supports31kHz120Hz() && supports31kHz();
    case i768x576:
      return !config.avoid_interlaced && !forced31kHzOnly() && supports50Hz();
    case i640x480:
      return !config.avoid_interlaced && !forced31kHzOnly() && !forced50Hz();
    case p640x480:
    case p1920x480:
      return supports31kHz();
    case p1920x384:
    case p496x384:
      return supports25kHz();
    default:
      return false;
  }
}

static int dpidac_get_modes(struct drm_connector *connector) {
  int i;
  dpidac_load_config(config_path);
  i = dpidac_load_timings(connector);
  if (i) {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: dpidac_get_modes: %i custom modes loaded\n", i);
    return i;
  } else {
    int modecount = 0;
    int default_res = config.desktop_res;
    int desktopWidth, desktopHeight;
    bool desktopInterlaced = false;
    if(load_forced_desktop(&desktopWidth, &desktopHeight, &desktopInterlaced)){
      default_res = desktopHeight;
      char modeStr[128];
      if (desktopInterlaced && config.avoid_interlaced)
        desktopInterlaced = false;
      sprintf(modeStr, "%c%dx%d", desktopInterlaced ? 'i' : 'p', desktopWidth, desktopHeight);
      int modeId = modeNameToModeId(modeStr);
      if(modeId != -1){
        printk(KERN_INFO "[RECALBOXRGBDUAL]: setting special mode %s\n", modeStr);
        if(is_compatible_with_current_config(modeId)){
          dpidac_apply_module_mode(connector, modeId, true);
          return 1;
        }
      }
    }
    if (default_res == 0) {
      default_res = ml_default_resolution(config.screentype, config.current_hat, config.gpioIn31kHz.gpio_state);
    }
    // 31khz modes
    if (supports31kHz()) {
      // if gpio is low, we add modes even if mode = auto
      printk(KERN_INFO "[RECALBOXRGBDUAL]: 31kHz modes will be available\n");
      dpidac_apply_module_mode(connector, p640x480, config.screentype == KHz31 || default_res != 240);
      dpidac_apply_module_mode(connector, p1920x480, false);
      modecount += 2;
      if(supports31kHz120Hz()){
        printk(KERN_INFO "[RECALBOXRGBDUAL]: 31kHz 240p@120Hz mode will be available\n");
        dpidac_apply_module_mode(connector, p1920x240at120, false);
        modecount += 1;
      }
    }

    // 24kHz modes
    if (supports25kHz()){
      printk(KERN_INFO "[RECALBOXRGBDUAL]: Multisync: 24kHz mode will be available\n");
      dpidac_apply_module_mode(connector, p1920x384, config.screentype == KHzMulti1525 && default_res != 240);
      modecount += 1;
    }

    // 15kHz modes (avoided only on 31kHz only screens)
    if(!forced31kHzOnly()) {
      if (forced50Hz() && supports50Hz()) {
        // 50hz
        printk(KERN_INFO "[RECALBOXRGBDUAL]: only 50Hz modes will be available\n");
        dpidac_apply_module_mode(connector, p384x288, default_res != 576);
        dpidac_apply_module_mode(connector, p1920x288, false);
        modecount += 2;
        if(!config.avoid_interlaced) {
          dpidac_apply_module_mode(connector, i768x576, default_res == 576);
          modecount += 1;
          return modecount;
        }
        return modecount;
      } else {
        printk(KERN_INFO "[RECALBOXRGBDUAL]: 60Hz%s modes will be available\n",
               supports50Hz() ? " + 50Hz" : "");
        dpidac_apply_module_mode(connector, isRGBJAMMA() ? p320x240jamma: p320x240, default_res == 240);
        dpidac_apply_module_mode(connector, p1920x240, false);
        dpidac_apply_module_mode(connector, p1920x224, false);
        modecount += 3;
        if (supports50Hz()) {
          dpidac_apply_module_mode(connector, p384x288, default_res == 288);
          dpidac_apply_module_mode(connector, p1920x288, false);
          modecount += 2;
        }
        if(!config.avoid_interlaced && (config.screentype == KHz15 || config.screentype == Auto)) {
          dpidac_apply_module_mode(connector, i640x480, default_res == 480);
          if (supports50Hz())
            dpidac_apply_module_mode(connector, i768x576, default_res == 576);
          modecount += supports50Hz() ? 2 : 1;
        }
        return modecount;
      }
    }
    // Should not happen but just in case
    if(modecount == 0){
        dpidac_apply_module_mode(connector, p320x240, true);
      return 1;
    }
    return modecount;
  }
}

static const struct drm_connector_helper_funcs dpidac_con_helper_funcs = {
    .get_modes    = dpidac_get_modes,
};

static enum drm_connector_status dpidac_connector_detect(struct drm_connector *connector, bool force) {
  return connector_status_connected;
}

static const struct drm_connector_funcs dpidac_con_funcs = {
    .detect            = dpidac_connector_detect,
    .fill_modes        = drm_helper_probe_single_connector_modes,
    .destroy        = drm_connector_cleanup,
    .reset            = drm_atomic_helper_connector_reset,
    .atomic_duplicate_state    = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state    = drm_atomic_helper_connector_destroy_state,
};

static int dpidac_attach(struct drm_bridge *bridge, enum drm_bridge_attach_flags flags) {
  struct dpidac *vga = drm_bridge_to_dpidac(bridge);
  u32 bus_format = MEDIA_BUS_FMT_RGB666_1X18;
  u32 mode;
  int ret;

  if (!bridge->encoder) {
    DRM_ERROR("Missing encoder\n");
    return -ENODEV;
  }

  drm_connector_helper_add(&vga->connector,
                           &dpidac_con_helper_funcs);
  ret = drm_connector_init(bridge->dev, &vga->connector,
                           &dpidac_con_funcs, DRM_MODE_CONNECTOR_VGA);
  if (ret) {
    DRM_ERROR("Failed to initialize connector\n");
    return ret;
  }

  of_property_read_u32(vga->bridge.of_node, "vc4-vga666-mode", &mode);
  printk(KERN_INFO "[RECALBOXRGBDUAL]: vc4-vga666 mode: %i\n", mode);
  if (mode == 6) {
    bus_format = MEDIA_BUS_FMT_RGB666_1X24_CPADHI;
  } else if (mode == 8) {
    bus_format = MEDIA_BUS_FMT_RGB888_1X24;
  }

  ret = drm_display_info_set_bus_formats(&vga->connector.display_info,
                                         &bus_format, 1);
  if (ret) {
    DRM_ERROR("Failed to set bus format\n");
    return ret;
  }

  vga->connector.interlace_allowed = 1;
  vga->connector.doublescan_allowed = 1;

  drm_connector_attach_encoder(&vga->connector,
                               bridge->encoder);

  return 0;
}

static const struct drm_bridge_funcs dpidac_bridge_funcs = {
    .attach        = dpidac_attach,
};

static int watch_configuration(void *idx) {
  while (!kthread_should_stop()) {
    usleep_range(2000000, 5000000);
    // Read file
    dpidac_load_config(config_path);
  }
  return 0;
}

static bool configureAudioOutput(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    /* OUT audio_enable */
    config.gpioOutAudioEnable.gpio = devm_gpiod_get_index_optional(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutAudioEnable.gpio)) {
      pr_err("V2 - Error when assigning gpioOutAudioEnable GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Enabling audio!\n");
      gpiod_export(config.gpioOutAudioEnable.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-audio-enable", config.gpioOutAudioEnable.gpio);
    }
    return true;
}

static int configureVideoFilterBypass(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutVideoFilterBypass.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if (IS_ERR(config.gpioOutVideoFilterBypass.gpio)) {
      pr_err("Error when assigning gpioOutVideoFilterBypass GPIO.\n");
      return false;
    } else {
      config.gpioOutVideoFilterBypass.gpio_state = gpiod_get_value_cansleep(config.gpioOutVideoFilterBypass.gpio);
      gpiod_export(config.gpioOutVideoFilterBypass.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-video-filter-bypass", config.gpioOutVideoFilterBypass.gpio);
    }
    return true;
}

static int configureDipswitch50Hz(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioIn50Hz.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if (IS_ERR(config.gpioIn50Hz.gpio)) {
      pr_err("Error when assigning gpioIn50Hz GPIO.\n");
      return false;
    } else {
      config.gpioIn50Hz.gpio_state = gpiod_get_value_cansleep(config.gpioIn50Hz.gpio);
      gpiod_export(config.gpioIn50Hz.gpio, false);
      gpiod_export_link(&pdev->dev, "dipswitch-50hz", config.gpioIn50Hz.gpio);
    }
    return true;
}

static int configureDipswitch31kHz(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioIn31kHz.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if (IS_ERR(config.gpioIn31kHz.gpio)) {
      pr_err("Error when assigning gpioIn31kHz GPIO.\n");
      return false;
    } else {
      config.gpioIn31kHz.gpio_state = gpiod_get_value_cansleep(config.gpioIn31kHz.gpio);
      gpiod_export(config.gpioIn31kHz.gpio, false);
      gpiod_export_link(&pdev->dev, "dipswitch-31khz", config.gpioIn31kHz.gpio);
    }
    return true;
}


static int configureRGBBlanking(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutRGBBlanking.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutRGBBlanking.gpio)) {
      pr_err("V2 - Error when assigning gpioOutRGBBlanking GPIO.\n");
      return false;
    } else {
      gpiod_export(config.gpioOutRGBBlanking.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-rgblanking", config.gpioOutRGBBlanking.gpio);
    }
    return true;
}


static int configureCompositeCSync(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutComposite.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutComposite.gpio)) {
      pr_err("V2 - Error when assigning gpioOutComposite GPIO.\n");
      return false;
    } else {
      gpiod_export(config.gpioOutComposite.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-composite", config.gpioOutComposite.gpio);
    }
    return true;
}

static int configureOutCompositeOnJackOnlyProto1(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutJackComp.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutJackComp.gpio)) {
      pr_err("V2 - Error when assigning gpioOutJackComp GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Disabling gpioOutJackComp!\n");
      gpiod_set_value_cansleep(config.gpioOutJackComp.gpio, 0);
      gpiod_export(config.gpioOutJackComp.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-jackcomp", config.gpioOutJackComp.gpio);
    }
    return true;
}

static int configureDipswitchComposite(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioInComposite.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if (IS_ERR(config.gpioInComposite.gpio)) {
      pr_err("Error when assigning gpioInComposite GPIO.\n");
      return false;
    } else {
      config.gpioInComposite.gpio_state = gpiod_get_value_cansleep(config.gpioInComposite.gpio);
      gpiod_export(config.gpioInComposite.gpio, false);
      gpiod_export_link(&pdev->dev, "dipswitch-composite", config.gpioInComposite.gpio);
      if (config.force_composite) {
		// Set the IN gpio to low to make composite system wide
		gpiod_direction_output(config.gpioInComposite.gpio, 0);
        config.gpioInComposite.gpio_state = 0;
      }
      if(config.gpioInComposite.gpio_state == 0){
        printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Forcing gpioOutComposite, disabling RGB.");
        gpiod_set_value_cansleep(config.gpioOutComposite.gpio, 1);
        gpiod_set_value_cansleep(config.gpioOutRGBBlanking.gpio, 1);
      } else {
        printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Enabling RGB.");
        gpiod_set_value_cansleep(config.gpioOutRGBBlanking.gpio, 0);
        gpiod_set_value_cansleep(config.gpioOutComposite.gpio, 0);
      }
    }
    return true;
}

static int configureWideScreen(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutWideScreen.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutWideScreen.gpio)) {
      pr_err("V2 - Error when assigning gpioOutWideScreen GPIO.\n");
      return false;
    } else {
      if(config.widescreen && !(config.gpioIn31kHz.gpio_state == 0 || config.screentype > KHz15)) {
        printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Enabling widescreen.\n");
      } else {
        printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Disabling widescreen.\n");
        config.widescreen = false;
      }
      gpiod_set_value_cansleep(config.gpioOutWideScreen.gpio, config.widescreen);
      gpiod_export(config.gpioOutWideScreen.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-widescreen", config.gpioOutWideScreen.gpio);
    }
    return true;
}
static int configureScartDisable(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutScartDisable.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutScartDisable.gpio)) {
      pr_err("V2 - Error when assigning gpioOutScartDisable GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Enabling Scart.\n");
      gpiod_set_value_cansleep(config.gpioOutScartDisable.gpio, 0);
      gpiod_export(config.gpioOutScartDisable.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-scart-disable", config.gpioOutScartDisable.gpio);
    }
    return true;
}

static int configureDacEnable(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutDacEnable.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutDacEnable.gpio)) {
      pr_err("V2 - Error when assigning gpioOutDacEnable GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Enabling dac!\n");
      gpiod_export(config.gpioOutDacEnable.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-dacenable", config.gpioOutDacEnable.gpio);
    }
    return true;
}

static int configureRPI5Csync(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutRPI5CSync.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutRPI5CSync.gpio)) {
      pr_err("V2 - Error when assigning gpioOutRPI5CSync GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Disabling rpi5 csync!\n");
      gpiod_set_value_cansleep(config.gpioOutRPI5CSync.gpio, 0);
      gpiod_export(config.gpioOutRPI5CSync.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-rpi5csync", config.gpioOutRPI5CSync.gpio);
    }
    return true;
}

static int configurePioVSCsyncComp(struct platform_device *pdev, const char * gpioreg, int gpioNum, int direction){
    config.gpioOutPioSync.gpio = devm_gpiod_get_index(&(pdev->dev), gpioreg, gpioNum, direction);
    if(IS_ERR(config.gpioOutPioSync.gpio)) {
      pr_err("V2 - Error when assigning gpioOutPioSync GPIO.\n");
      return false;
    } else {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - Selecting csync/comp on gpioOutPioSync!\n");
      gpiod_set_value_cansleep(config.gpioOutPioSync.gpio, 1);
      gpiod_export(config.gpioOutPioSync.gpio, false);
      gpiod_export_link(&pdev->dev, "rgbd2-piovscsync", config.gpioOutPioSync.gpio);
    }
    return true;
}

static int power_off(struct sys_off_data *data){
  if(config.gpioOutScartDisable.gpio){
    gpiod_set_value_cansleep(config.gpioOutScartDisable.gpio, 1);
  }
  if(config.gpioOutRGBBlanking.gpio){
    gpiod_set_value_cansleep(config.gpioOutRGBBlanking.gpio, 1);
  }
  if(config.gpioOutWideScreen.gpio){
    gpiod_set_value_cansleep(config.gpioOutWideScreen.gpio, 1);
  }
  if(config.gpioOutRPI5CSync.gpio){
    gpiod_set_value_cansleep(config.gpioOutRPI5CSync.gpio, 1);
  }
  if(config.gpioOutComposite.gpio){
    gpiod_set_value_cansleep(config.gpioOutComposite.gpio, 1);
  }
  return 0;
}

#define RPI_GPIO "rp1"
#define PCA_GPIO "pca"

static int dpidac_probe(struct platform_device *pdev) {
  printk(KERN_INFO "[RECALBOXRGBDUAL]: Probe OK, setting state to plugged\n");
  config.unplugged = false;
  struct dpidac *vga;
  u32 rgbdual = 0;
  u32 rgbdual2_rev = 0;
  u32 rgbjamma = 0;
  u32 rgbjamma2 = 0;
  u32 jvs = 0;
  u32 avoid_interlaced = 0;

  vga = devm_kzalloc(&pdev->dev, sizeof(*vga), GFP_KERNEL);
  if (!vga)
    return -ENOMEM;
  platform_set_drvdata(pdev, vga);

  vga->bridge.funcs = &dpidac_bridge_funcs;
  vga->bridge.of_node = pdev->dev.of_node;

  of_property_read_u32(vga->bridge.of_node, "recalbox-rgb-dual", &rgbdual);
  of_property_read_u32(vga->bridge.of_node, "recalbox-rgb-dual-2", &rgbdual2_rev);
  of_property_read_u32(vga->bridge.of_node, "recalbox-rgb-jamma", &rgbjamma);
  of_property_read_u32(vga->bridge.of_node, "recalbox-rgb-jamma-2", &rgbjamma2);
  of_property_read_u32(vga->bridge.of_node, "recalbox-jvs", &jvs);

  printk(KERN_INFO "[RECALBOXRGBDUAL]: jvs = %d!\n", jvs);

  config.desktop_res = 0;
  config.screentype = Auto;
  config.widescreen = false;
  config.force_composite = false;

  config.avoid_interlaced = false;

  config.gpioIn50Hz.gpio_state = 1;
  config.gpioIn31kHz.gpio_state = 1;
  config.config_thread = NULL;

  of_property_read_u32(vga->bridge.of_node, "avoid_interlaced", &avoid_interlaced);
  if (avoid_interlaced == 1) {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Disabling interlaced modes\n");
    config.avoid_interlaced = true;
  }

  if (rgbdual == 1) {
    config.current_hat = RecalboxRGBDual;
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Thank you for your support, have fun on Recalbox RGB DUAL!\n");

    /* Switch 31kHz */
    config.gpioIn31kHz.gpio = devm_gpiod_get_index(&(pdev->dev), "dipswitch", 0, GPIOD_IN);
    if (IS_ERR(config.gpioIn31kHz.gpio)) {
      pr_err("Error when assigning gpioIn31kHz GPIO.\n");
    } else {
      config.gpioIn31kHz.gpio_state = gpiod_get_value(config.gpioIn31kHz.gpio);
      gpiod_export(config.gpioIn31kHz.gpio, false);
      gpiod_export_link(&pdev->dev, "dipswitch-31khz", config.gpioIn31kHz.gpio);
    }

    /* Switch 50 HZ */
    config.gpioIn50Hz.gpio = devm_gpiod_get_index(&(pdev->dev), "dipswitch", 1, GPIOD_IN);
    if (IS_ERR(config.gpioIn50Hz.gpio)) {
      pr_err("Error when assigning gpioIn50Hz GPIO.\n");
    } else {
      config.gpioIn50Hz.gpio_state = gpiod_get_value(config.gpioIn50Hz.gpio);
      gpiod_export(config.gpioIn50Hz.gpio, false);
      gpiod_export_link(&pdev->dev, "dipswitch-50hz", config.gpioIn50Hz.gpio);
    }

    printk(KERN_INFO "[RECALBOXRGBDUAL]: gpioIn50Hz: %i, gpioIn31kHz: %i\n", config.gpioIn50Hz.gpio_state, config.gpioIn31kHz.gpio_state);

  } else if(rgbdual2_rev > 0) {
    config.current_hat = RecalboxRGBDual2;
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Thank you for your support, have fun on Recalbox RGB DUAL 2!\n");

    dpidac_load_config(config_path);
    int idx = 0;
    config.config_thread = kthread_create(watch_configuration, &idx, "kthread_recalboxrgbdual_cfg");
    printk(KERN_INFO "[RECALBOXRGBDUAL]: setting configuration thread\n");
    if (config.config_thread != NULL) {
      wake_up_process(config.config_thread);
      printk(KERN_INFO "[RECALBOXRGBDUAL]: kthread_recalboxrgbdual_cfg is running\n");
    } else {
      printk(KERN_ERR "[RECALBOXRGBDUAL]: kthread kthread_recalboxrgbdual_cfg could not be created\n");
    }
    if(rgbdual2_rev == 1) {
      /* OUT RGB Blanking */
      configureRGBBlanking(pdev, RPI_GPIO, 0, GPIOD_OUT_LOW);
      /* Dac enable */
      configureDacEnable(pdev, RPI_GPIO, 1, GPIOD_OUT_HIGH);
      /* IN Switch 31kHz */
      configureDipswitch31kHz(pdev, PCA_GPIO, 3, GPIOD_IN);
      /* OUT Wide screen - Must Be After 31Khz dipswitch */
      configureWideScreen(pdev, PCA_GPIO, 0, GPIOD_OUT_LOW);
      /* OUT gpioOutComposite / CSYNC */
      configureCompositeCSync(pdev, PCA_GPIO, 1, GPIOD_OUT_LOW);
      /* Jack on gpioOutComposite */
      configureOutCompositeOnJackOnlyProto1(pdev, PCA_GPIO, 5, GPIOD_OUT_LOW);
      /* IN Switch 50 HZ */
      configureDipswitch50Hz(pdev, PCA_GPIO, 2, GPIOD_IN);
      /* IN Switch Composite - Must be after Composite/Csync and RGB Blanking */
      configureDipswitchComposite(pdev, PCA_GPIO, 4, GPIOD_IN);

      printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 - gpioInComposite: %i, gpioIn50Hz: %i, gpioIn31kHz: %i, \n", config.gpioInComposite.gpio_state, config.gpioIn50Hz.gpio_state, config.gpioIn31kHz.gpio_state);
    } else if(rgbdual2_rev == 2 || rgbdual2_rev == 4) {
      /* OUT audio_enable */
      configureAudioOutput(pdev, RPI_GPIO, 0, GPIOD_OUT_HIGH);
      /* OUT Video Filter Bypass */
      configureVideoFilterBypass(pdev, PCA_GPIO, 0, GPIOD_OUT_HIGH);
      /* IN Switch 50 HZ */
      configureDipswitch50Hz(pdev, PCA_GPIO, 1, GPIOD_IN);
      /* IN Switch 31kHz */
      configureDipswitch31kHz(pdev, PCA_GPIO, 2, GPIOD_IN);
      /* OUT RGB Blanking */
      configureRGBBlanking(pdev, PCA_GPIO, rgbdual2_rev == 4 ? 5 : 7, GPIOD_OUT_LOW);
      /* OUT gpioOutComposite / CSYNC */
      configureCompositeCSync(pdev, PCA_GPIO, rgbdual2_rev == 4 ? 4 : 5, GPIOD_OUT_LOW);
      /* IN Switch Composite - Must be after Composite/Csync and RGB Blanking */
      configureDipswitchComposite(pdev, PCA_GPIO, 3, GPIOD_IN);
      /* OUT Wide screen - Must Be After 31Khz dipswitch */
      if(rgbdual2_rev == 4){
        configureWideScreen(pdev, RPI_GPIO, 1, GPIOD_OUT_LOW);
      } else {
        configureWideScreen(pdev, PCA_GPIO, 4, GPIOD_OUT_LOW);
      }
      /* OUT Scart Disable */
      configureScartDisable(pdev,PCA_GPIO, rgbdual2_rev == 4 ? 7 : 6, GPIOD_OUT_LOW);
      /* Dac enable */
      configureDacEnable(pdev,PCA_GPIO, rgbdual2_rev == 4 ? 6 : 8, GPIOD_OUT_HIGH);
      /* RPI5 CSYNC */
      configureRPI5Csync(pdev,PCA_GPIO, rgbdual2_rev == 4 ? 8 : 9, GPIOD_OUT_LOW);
    } else if(rgbdual2_rev == 5 || rgbdual2_rev == 6) {
      /* OUT audio_enable */
      configureAudioOutput(pdev, RPI_GPIO, 0, GPIOD_OUT_HIGH);
      /* OUT Video Filter Bypass */
      configureVideoFilterBypass(pdev, PCA_GPIO, 0, GPIOD_OUT_HIGH);
      /* IN Switch 50 HZ */
      configureDipswitch50Hz(pdev, PCA_GPIO, 1, GPIOD_IN);
      /* IN Switch 31kHz */
      configureDipswitch31kHz(pdev, PCA_GPIO, 2, GPIOD_IN);
      /* OUT RGB Blanking */
      configureRGBBlanking(pdev, PCA_GPIO, 5, GPIOD_OUT_LOW);
      /* OUT gpioOutComposite / CSYNC */
      configureCompositeCSync(pdev, PCA_GPIO, 4, GPIOD_OUT_LOW);
      /* IN Switch Composite - Must be after Composite/Csync and RGB Blanking */
      configureDipswitchComposite(pdev, PCA_GPIO, 3, GPIOD_IN);
      /* OUT Wide screen - Must Be After 31Khz dipswitch */
      configureWideScreen(pdev, PCA_GPIO, 9, GPIOD_OUT_LOW);
      /* OUT Scart Disable */
      configureScartDisable(pdev,PCA_GPIO, 7, GPIOD_OUT_LOW);
      /* Dac enable */
      configureDacEnable(pdev,PCA_GPIO, 6, GPIOD_OUT_HIGH);
      /* RPI5 CSYNC */
      configureRPI5Csync(pdev,PCA_GPIO, 8, GPIOD_OUT_LOW);
      /* PIO VS CSYNC/Comp selection  */
      configurePioVSCsyncComp(pdev,PCA_GPIO, 10, GPIOD_OUT_LOW);
    }

    printk(KERN_INFO "[RECALBOXRGBDUAL]: V2 (rev-%d) - gpioInComposite: %i, gpioIn50Hz: %i, gpioIn31kHz: %i, gpioOutAudioEnable: %i,"
                     " gpioOutVideoFilterBypass: %i, gpioOutRGBBlanking: %i, gpioOutComposite: %i, gpioOutWideScreen: %i,"
                     " gpioOutScartDisable: %i, gpioOutDacEnable: %i, gpioOutRPI5CSync: %i, gpioOutPioSync: %i\n",
          rgbdual2_rev,
          config.gpioInComposite.gpio_state,
          config.gpioIn50Hz.gpio_state,
          config.gpioIn31kHz.gpio_state,
          gpiod_get_value_cansleep(config.gpioOutAudioEnable.gpio),
          gpiod_get_value_cansleep(config.gpioOutVideoFilterBypass.gpio),
          gpiod_get_value_cansleep(config.gpioOutRGBBlanking.gpio),
          gpiod_get_value_cansleep(config.gpioOutComposite.gpio),
          gpiod_get_value_cansleep(config.gpioOutWideScreen.gpio),
          gpiod_get_value_cansleep(config.gpioOutScartDisable.gpio),
          gpiod_get_value_cansleep(config.gpioOutDacEnable.gpio),
          gpiod_get_value_cansleep(config.gpioOutRPI5CSync.gpio),
          gpiod_get_value_cansleep(config.gpioOutPioSync.gpio));
      register_sys_off_handler(SYS_OFF_MODE_POWER_OFF, SYS_OFF_PRIO_DEFAULT, power_off, NULL);

  } else if(rgbjamma == 1) {
      printk(KERN_INFO "[RECALBOXRGBDUAL]: Thank you for your support, have fun on Recalbox RGB JAMMA!\n");
      config.current_hat = RecalboxRGBJAMMA;
  } else if(rgbjamma2 == 1) {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Thank you for your support, have fun on Recalbox RGB JAMMA 2!\n");
    config.current_hat = RecalboxRGBJAMMA2;
  } else if( jvs == 1 ) {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Thank you for your support, have fun on Recalbox JVS!\n");
    config.current_hat = RecalboxJVS;
    // Config is loaded, if screentype = Auto we are 31kHz only
    if(config.screentype == Auto){
      config.screentype = KHz31;
    }
  } else {
    printk(KERN_INFO "[RECALBOXRGBDUAL]: Third party DAC.\n");
    config.current_hat = OTHER;
  }
  drm_bridge_add(&vga->bridge);

  return 0;
}

static void dpidac_remove(struct platform_device *pdev) {
  if (config.config_thread != NULL && kthread_stop(config.config_thread)) {
    printk("[RECALBOXRGBDUAL]: can't stop config thread");
  }

  struct dpidac *vga = platform_get_drvdata(pdev);
  drm_bridge_remove(&vga->bridge);
}

static const struct of_device_id dpidac_match[] = {
    {.compatible = "raspberrypi,recalboxrgbdual"},
    {},
};
MODULE_DEVICE_TABLE(of, dpidac_match);

static struct platform_driver dpidac_driver = {
    .probe  = dpidac_probe,
    .remove = dpidac_remove,
    .driver = {
        .name        = "recalboxrgbdual",
        .of_match_table    = dpidac_match,
    },
};


static ssize_t rgbdual2_plugged_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
  return sysfs_emit(buf, config.unplugged ? "false" : "true");
}

static struct kobj_attribute rgbdual_plugged_attr = __ATTR(rgbdual_plugged, 0444, rgbdual2_plugged_show, NULL);

static ssize_t interlaced_supported_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
  return sysfs_emit(buf, config.avoid_interlaced ? "false" : "true");
}

static struct kobj_attribute interlaced_supported_attr = __ATTR(interlaced_supported, 0444, interlaced_supported_show, NULL);
static struct kobject *rgbdual_kobj;

//module_platform_driver(dpidac_driver);
static int __init rgbdual_init(void)
{
  printk(KERN_INFO "[RECALBOXRGBDUAL]: Init\n");
  config.unplugged = true;
  rgbdual_kobj = kobject_create_and_add("recalboxrgbdual2", kernel_kobj);
  if(!rgbdual_kobj) {
    printk(KERN_ERR "recalboxrgbjamma: unable to create /sys/kernel/recalboxrgbdual2\n");
  } else {
    if(sysfs_create_file(rgbdual_kobj, &rgbdual_plugged_attr.attr)) {
      printk(KERN_ERR "recalboxrgbjamma: unable to create /sys/kernel/recalboxrgbdual2/rgbjamma_plugged\n");
      kobject_put(rgbdual_kobj);
    }
    if(sysfs_create_file(rgbdual_kobj, &interlaced_supported_attr.attr)) {
      printk(KERN_ERR "recalboxrgbdual: unable to create /sys/kernel/recalboxrgbdual2/interlaced_supported\n");
    }
  }
  return platform_driver_register(&dpidac_driver);
}

static void __exit rgbdual_exit(void)
{
  printk(KERN_INFO "[RECALBOXRGBDUAL]: Exit\n");
  platform_driver_unregister(&dpidac_driver);
}

module_init(rgbdual_init);
module_exit(rgbdual_exit);

MODULE_AUTHOR("Hugh Cole-Baker and cpasjuste and digitalLumberjack");
MODULE_DESCRIPTION("Raspberry Pi Recalbox RGB Dual driver");
MODULE_LICENSE("GPL");
