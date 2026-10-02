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
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "fd.h"
#include "log.h"

#define __USE_GNU 1

volumed_fd * new_input_device(char * path, int flags) {
  char name[512];
  int fd = open(path, flags);
  if (!fd)
    return NULL;
  volumed_fd * vid = malloc(sizeof(volumed_fd));
  if (vid) {
    vid->type = FD_INPUT_DEVICE;
    vid->fd = fd;
    vid->path = strdup(path);
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0)
      vid->name = strdup(name);
    else
      vid->name = NULL;
    _syslog(LOG_DEBUG, "device %s (fd=%d, name=%s) opened", path, fd, name);
  }else {
    close(fd);
    if (!errno)
      errno = ENOMEM;
  }
  return vid;
}

volumed_fd * new_signal_fd(const int * signals) {
  sigset_t mask;
  sigemptyset(&mask);
  while(*signals) {
    _syslog(LOG_INFO, "adding signal SIG%s", sigabbrev_np(*signals));
    sigaddset(&mask, *signals++);
  }
  if (sigprocmask(SIG_BLOCK, &mask, 0) == -1)
    return NULL;

  volumed_fd * vid = malloc(sizeof(volumed_fd));
  if (vid) {
    vid->type = FD_SIGNAL;
    vid->fd = signalfd(-1, &mask, 0);
    _syslog(LOG_DEBUG, "signal fd %d", vid->fd);
    vid->path = NULL;
    vid->name = NULL;
  }else {
    if (!errno)
      errno = ENOMEM;
  }
  return vid;
}

void free_fd(volumed_fd* vid) {
  int fd;
  if (vid) {
    fd = vid->fd;
    if (vid->fd)
      close(vid->fd);
    if (vid->name)
      free(vid->name);
    if (vid->path)
      free(vid->path);
    free(vid);
    _syslog(LOG_DEBUG, "device %d freed", fd);
  }
}

const char *ev_type_to_string(int type) {
  switch (type) {
    case EV_KEY: return "EV_KEY";
    case EV_ABS: return "EV_ABS";
    default: return "EV_UNKNOWN";
  }
}

const char *ev_code_to_string(int code) {
  switch (code) {
    case ABS_VOLUME:         return "ABS_VOLUME";
    case KEY_VOLUMEUP:       return "KEY_VOLUMEUP";
    case KEY_VOLUMEDOWN:     return "KEY_VOLUMEDOWN";
    case KEY_BRIGHTNESSUP:   return "KEY_BRIGHTNESSUP";
    case KEY_BRIGHTNESSDOWN: return "KEY_BRIGHTNESSDOWN";
    default:                 return "UNKNOWN";
  }
}
