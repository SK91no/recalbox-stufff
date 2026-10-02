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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "backlight.h"
#include "log.h"

#define BACKLIGHT_PATH "/sys/class/backlight/"

static backlight g_backlight = {0};

// initilize our backlight
// TODO: only the first is used
int backlight_init() {
  struct dirent *entry;
  DIR *dir;

  if (!(dir = opendir(BACKLIGHT_PATH))) {
    perror("opendir");
    return -1;
  }

  // grab first found backlight
  while ((entry = readdir(dir)) != NULL) {
    if ((entry->d_type == DT_LNK || entry->d_type == DT_DIR) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

      snprintf(g_backlight.device_path, sizeof(g_backlight.device_path),
          "%s%s", BACKLIGHT_PATH, entry->d_name);
      break;
    }
  }
  closedir(dir);

  if (g_backlight.device_path[0] == 0) {
    _syslog(LOG_ERR, "no backlight found");
    return -1;
  }

  // get max_brightness
  char max_path[512];
  snprintf(max_path, sizeof(max_path), "%s/max_brightness", g_backlight.device_path);
  FILE *fmax = fopen(max_path, "r");
  if (!fmax) {
    perror("fopen max_brightness");
    return -1;
  }

  if (fscanf(fmax, "%d", &g_backlight.max_brightness) != 1) {
    _syslog(LOG_ERR, "failed to read max_brightness");
    fclose(fmax);
    return -1;
  }
  fclose(fmax);

  return 0;
}

char * forge_backlight_path(char * buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "%s/brightness", g_backlight.device_path);
  return buffer;
}
// adjust brightness level
// level is an integer between 0 and 7
int backlight_set_level(int level) {
  if (level < 0 || level > 8) {
    _syslog(LOG_ERR, "level must be between 0 and 8");
    return -1;
  }

  // silently do nothing if no backlight
  if (g_backlight.device_path[0] == 0) {
    return -1;
  }

  // map 0-8 values to 0-max_brightness
  int value = (int)rintf(level * g_backlight.max_brightness / 8.0);

  char brightness_path[512];
  forge_backlight_path(brightness_path, sizeof(brightness_path));

  FILE *fbright = fopen(brightness_path, "w");
  if (!fbright) {
    _syslog(LOG_ERR, "cant fopen brightness %s: %s", brightness_path, strerror(errno));
    return -1;
  }

  fprintf(fbright, "%d\n", value);
  fclose(fbright);

  return 0;
}

int backlight_get_level() {
  int level = -1;
  char buffer[16];
  char brightness_path[512];

  // silently do nothing if no backlight
  if (g_backlight.device_path[0] == 0) {
    return -1;
  }

  forge_backlight_path(brightness_path, sizeof(brightness_path));
  FILE *backlight = fopen(brightness_path, "r");

  if (backlight) {
    fgets(buffer, 16, backlight);
    level = atoi(buffer);
    level = (int) rintf(level * 8.0 / g_backlight.max_brightness);
    fclose(backlight);
  }else
    _syslog(LOG_ERR, "cant fopen brightness %s: %s", brightness_path, strerror(errno));
  return level;
}
