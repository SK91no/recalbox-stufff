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

#ifndef RECALBOX_VOLUMED_FD_H
#define RECALBOX_VOLUMED_FD_H

#include <stdio.h>

typedef enum _fd_type {
  FD_INPUT_DEVICE,
  FD_SIGNAL,
} fd_type;

typedef struct _volumed_fd {
  int fd;
  char * name;
  char * path;
  fd_type type;
} volumed_fd;

volumed_fd * new_input_device(char * path, int flags);
volumed_fd * new_signal_fd(const int * signals);
void free_fd(volumed_fd* vid);
const char *ev_type_to_string(int type);
const char *ev_code_to_string(int code);

#endif
