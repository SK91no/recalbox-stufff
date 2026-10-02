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

#include <linux/input.h>

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "volumed.h"

long recalbox_conf_read_integer(char * key, long default_value) {
  char cmdline[255];
  FILE *fp;
  char data[8];
  long value = default_value;;
  snprintf(cmdline, sizeof(cmdline), "/usr/bin/recalbox_settings -command load -key %s -default %ld", key, default_value);

  fp = popen(cmdline, "r");
  if (fp) {
    // read first line
    if (fgets(data, sizeof(data), fp))
      value = atoi(data);
  }

  pclose(fp);
  return value;
}

int recalbox_conf_save(char * key, long value) {
  char cmdline[255];
  snprintf(cmdline, sizeof(cmdline), "/usr/bin/recalbox_settings -command save -key %s -value %ld", key, value);
  return system(cmdline);
}

/**
 * save volume & brightness values to recalbox.conf
 */
void write_recalbox_values() {
  recalbox_conf_save("audio.volume", gVolume);
  recalbox_conf_save("emulationstation.brightness", gBrightness);
  _syslog(LOG_INFO, "set volume to %d and brightness to %ld in recalbox.conf", gVolume, (long)gBrightness);
}

/**
 * read volume value from recalbox.conf
 */
void read_recalbox_values() {
  gVolume = recalbox_conf_read_integer("audio.volume", DEFAULT_VOLUME);
  gBrightness = recalbox_conf_read_integer("emulationstation.brightness", DEFAULT_BRIGHTNESS);

  _syslog(LOG_INFO, "initial volume set to %ld and brightness set to %ld", gVolume, gBrightness);
}

