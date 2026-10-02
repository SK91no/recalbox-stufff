/*
 * volumed - volume daemon for PiBoy DMG volume wheel input
 *
 * Copyright (C) 2021 - David Barbion <davidb@230ruedubac.fr>
 *
 * based on evtest.c,  Copyright (c) 1999-2000 Vojtech Pavlik
 *          amixer.c,  Copyright (c) 1999-2000 by Jaroslav Kysela
 *          volumed.c, Copyright (c) 2005 by Jochen Eisinger
 *          pavolume.c,  Copyright (c) 2017-2021 Biniam Bekele (Andornaut)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define _GNU_SOURCE
#include <linux/input.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/select.h>
#include <sys/signalfd.h>

#include "audio.h"
#include "backlight.h"
#include "fd.h"
#include "io.h"
#include "log.h"
#include "parse.h"
#include "volumed.h"

#define TEST_BIT(array, bit) ((array)[(bit) / (8 * sizeof(unsigned long))] & (1UL << ((bit) % (8 * sizeof(unsigned long)))))

#define MAX_DEVICES 32
#define MAX_READ_EVENTS 256

int gVolume = DEFAULT_VOLUME;
long gBrightness = DEFAULT_BRIGHTNESS;
int gHKfd = -1;

// read an input device FD and if it contains
// ABS_VOLUME, get its value
// this will be the initial volume value
int get_abs_volume_from_fd(int fd) {
  unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
  int abs[5];

  memset(bit, 0, sizeof(bit));
  ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
  // query device for initial value
  if (test_bit(EV_ABS, bit[0])) {
    ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), bit[EV_ABS]);
    if (test_bit(ABS_VOLUME, bit[EV_ABS])) {
      ioctl(fd, EVIOCGABS(ABS_VOLUME), abs);
      return abs[0];
    }
  }
  return -1;
}

// check if it has required capabilities
bool has_required_capabilities(int fd) {
    unsigned long keybits[(KEY_MAX + 1) / (8 * sizeof(unsigned long))] = {0};
    unsigned long absbits[(ABS_MAX + 1) / (8 * sizeof(unsigned long))] = {0};

    // keys bitmap
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) return 0;
    // axes bitmap
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) < 0) return 0;

    if (TEST_BIT(keybits, KEY_VOLUMEUP) ||
        TEST_BIT(keybits, KEY_VOLUMEDOWN) ||
        TEST_BIT(keybits, KEY_BRIGHTNESSUP) ||
        TEST_BIT(keybits, KEY_BRIGHTNESSDOWN) ||
        TEST_BIT(absbits, ABS_VOLUME)) {
        return true;
    }

    return false;
}

// compare an opened input_device and a name
bool device_name_matches(int fd, const char *wanted) {
  char name[256] = "unknown";
  if (!wanted)
    return false;

  if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
    return false ;

  if (strcmp(name, wanted) == 0)
    return true;

  return false;
}

// open all interesting input_device and signals
// it is filtered on device that offers KEY_VOLUMEUP, KEY_VOLUMEDOWN,
// ABS_VOLUME, KEY_BRIGHTNESSDOWN and KEY_BRIGHTNESSUP
// Also opens a named input device that comes with the hotkey
// Finally opens signals (signalfd)
int open_fds(volumed_fd * vids[MAX_DEVICES], const char * hk_input_device) {
  int nfds = 0;
  DIR *dir = opendir("/dev/input");

  if (!dir) {
    perror("opendir");
    return 0;
  }

  const struct dirent *entry;
  while ((entry = readdir(dir)) && nfds < MAX_DEVICES) {
    if (strncmp(entry->d_name, "event", 5) == 0) {
      char path[256];
      snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
      volumed_fd * new_vid = new_input_device(path, O_RDONLY | O_NONBLOCK);
      if (new_vid && new_vid->fd >= 0) {
        if (has_required_capabilities(new_vid->fd) || device_name_matches(new_vid->fd, hk_input_device)) {
          vids[nfds++] = new_vid;
          //fds[nfds++] = fd;
          if (device_name_matches(new_vid->fd, hk_input_device)) {
            _syslog(LOG_INFO, "adding %s (fd %d, hk)", path, new_vid->fd);
            gHKfd = nfds-1;
          }else {
            _syslog(LOG_INFO, "adding %s (fd %d)", path, new_vid->fd);
          }
        }else {
          free_fd(new_vid);
        }
      }else {
        _syslog(LOG_ERR, "can't open %s: %s", path, strerror(errno));
      }
    }
  }
  closedir(dir);

  const int signals[] = {SIGTERM, SIGINT, SIGHUP, SIGALRM, 0};
  vids[nfds++] = new_signal_fd(signals);
  return nfds;
}

void close_fds(volumed_fd * vids[MAX_DEVICES], int nfds) {
  for(int i = 0; i < nfds; i++)
    free_fd(vids[i]);
}

void handle_input_device_event(volumed_struct_pa * volumed_pa, volumed_fd * v_fd, int nfd, bool * hk_pressed, struct input_event *ev) {
  if ((ev->type == EV_ABS) && (ev->code == ABS_VOLUME)) {
    if (ev->value > 100) {
      _syslog(LOG_WARNING, "event value over 100 ignored");
    }else {
      gVolume = ev->value;
    }
  }

  if (ev->type == EV_KEY) {
    if (nfd == gHKfd && ev->code == arguments.hotkey_code && ev->value == 1)
      *hk_pressed = true;
    if (nfd == gHKfd && ev->code == arguments.hotkey_code && ev->value == 0)
      *hk_pressed = false;
    if (*hk_pressed || ev->code == KEY_BRIGHTNESSUP || ev->code == KEY_BRIGHTNESSDOWN) {
      gBrightness = backlight_get_level();
      if ((ev->code == KEY_VOLUMEUP || ev->code == KEY_BRIGHTNESSUP)
          && (ev->value != 1) && (gBrightness <= MAX_BRIGHTNESS - BRIGHTNESS_STEP)) {
        gBrightness += BRIGHTNESS_STEP;
        backlight_set_level(gBrightness);
      }
      if ((ev->code == KEY_VOLUMEDOWN || ev->code == KEY_BRIGHTNESSDOWN)
          && (ev->value != 1) && (gBrightness >= MIN_BRIGHTNESS + BRIGHTNESS_STEP)) {
        gBrightness -= BRIGHTNESS_STEP;
        backlight_set_level(gBrightness);
      }
    }else {
      gVolume = get_audio_volume(volumed_pa);
      if ((ev->code == KEY_VOLUMEUP) && (ev->value != 1) && (gVolume <= MAX_VOLUME - VOLUME_STEP)) {
        gVolume += VOLUME_STEP;
      }
      if ((ev->code == KEY_VOLUMEDOWN) && (ev->value != 1) && (gVolume >= MIN_VOLUME + VOLUME_STEP)) {
        gVolume -= VOLUME_STEP;
      }
    }
  }
  // when an interesting key code has triggered
  // initialize alrm timeout to save our volume and brightness values
  if (((ev->type == EV_ABS) && (ev->code == ABS_VOLUME)) ||
      ((ev->type == EV_KEY) && ev->value != 1 &&
       (ev->code == KEY_VOLUMEUP || ev->code == KEY_VOLUMEDOWN ||
        ev->code == KEY_BRIGHTNESSUP || ev->code == KEY_BRIGHTNESSDOWN))) {
    alarm(1);
    set_audio_volume(volumed_pa, gVolume);
    _syslog(LOG_DEBUG, "received event type %s with code %s from %s value is %d", ev_type_to_string(ev->type), ev_code_to_string(ev->code), v_fd->name, ev->value);
  }

}

int main(int argc, char *argv[argc])
{
  int rd;
  volumed_fd * v_fds[MAX_DEVICES];
  int nfds = 0;
  struct input_event ev[MAX_READ_EVENTS];
  struct signalfd_siginfo si[MAX_READ_EVENTS];
  bool hk_pressed = false, reload_devices = false;
  volumed_struct_pa * volumed_pa;

  if (!parse_options(argc, argv))
    return 1;

  _openlog("volumed", 0, LOG_DAEMON, arguments.debug ? LOG_DEBUG : LOG_INFO);

  _syslog(LOG_INFO, "hotkey device name: %s", arguments.hotkey_inputdevice_name);
  _syslog(LOG_INFO, "hotkey code: %d", arguments.hotkey_code);

  read_recalbox_values();

  // daemon control
  if (!arguments.foreground) {
    if (fork())
      return 0;

    close(0);
    close(1);
    close(2);
  }

  // open all needed fds
  nfds = open_fds(v_fds, arguments.hotkey_inputdevice_name);


  // initialize backlight
  backlight_init();
  backlight_set_level(gBrightness);

  // initialize pulseaudio
  if (!(volumed_pa = initialize_audio())) {
      free_audio(volumed_pa);
      return EXIT_FAILURE;
  }

  // set initial volume if asked to do so
  if (arguments.use_abs_volume) {
    for(int i=0; i<nfds; i++) {
      int v = get_abs_volume_from_fd(v_fds[i]->fd);
      if (v>=0) {
        gVolume = v;
        _syslog(LOG_INFO, "got ABS_VOLUME %d from device %s", gVolume, v_fds[i]->name);
      }
    }
  }else {
    // or get current volume of the sink
    gVolume = get_audio_volume(volumed_pa);
  }

  _syslog(LOG_NOTICE, "volumed initialized");
  while (1) {
    /* prepare fds for select
     */
    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfd=0;
    for (int i =0; i < nfds; i++) {
      FD_SET(v_fds[i]->fd, &readfds);
      if (v_fds[i]->fd > maxfd)
        maxfd = v_fds[i]->fd;
    }

    // wait for events
    int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      _syslog(LOG_ERR, "select: %s", strerror(errno));
      break;
    }

    for (int i = 0; i < nfds; i++) {
      if (FD_ISSET(v_fds[i]->fd, &readfds)) {

        /* handle input device event
         */
        if (v_fds[i]->type == FD_INPUT_DEVICE) {
          _syslog(LOG_DEBUG, "input device event received");
          rd = read(v_fds[i]->fd, ev, sizeof(struct input_event) * MAX_READ_EVENTS);
          if (rd > 0) {
            for (int j = 0; j < rd / sizeof(struct input_event); j++) {
              handle_input_device_event(volumed_pa, v_fds[i], i, &hk_pressed, &ev[j]);
            }
          }
        }

        /* handle signals
         */
        if (v_fds[i]->type == FD_SIGNAL) {
          rd = read(v_fds[i]->fd, si, sizeof(struct signalfd_siginfo) * MAX_READ_EVENTS);
          if (rd > 0) {
            for (int j = 0; j < rd / sizeof(struct signalfd_siginfo); j++) {
              _syslog(LOG_DEBUG, "signal SIG%s received", sigabbrev_np(si[j].ssi_signo));
              switch(si[j].ssi_signo) {
                case SIGINT:
                case SIGTERM:
                  goto exit
                  ;;
                case SIGHUP:
                  // device reloading should be done outside of the main for loop
                  reload_devices = true;
                  break;
                case SIGALRM:
                  // write back values to configuration file
                  write_recalbox_values();
                  // stop alarming
                  alarm(0);
                  break;
              }
            }
          }
        }
      }
    }
    if (reload_devices) {
      reload_devices = false;
      _syslog(LOG_NOTICE, "reload input devices");
      gHKfd = -1;
      close_fds(v_fds, nfds);
      nfds = open_fds(v_fds, arguments.hotkey_inputdevice_name);
    }
  }
exit:
  // close all fds
  close_fds(v_fds, nfds);
  _syslog(LOG_INFO, "volumed exited");
  return EXIT_SUCCESS;
}
