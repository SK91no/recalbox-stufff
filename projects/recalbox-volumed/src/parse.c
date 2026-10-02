
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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

volumed_arguments arguments;

void usage(char * exec){
  fprintf(stderr, "Usage: %s [-d] [-H NAME:CODE]\n", exec);
  fprintf(stderr, "\n");
  fprintf(stderr, "  -d, --debug                     enable debug\n");
  fprintf(stderr, "  -f, --foreground                run volumed in foreground\n");
  fprintf(stderr, "  -A, --enable-abs-volume         if any devices return ABS_VOLUME, use it\n");
  fprintf(stderr, "  -H, --hotkey-code NAME:CODE     set input_device NAME and KEY_CODE for the hotkey\n");
}

bool parse_options(int argc, char *argv[]) {
  int opt;

  // clear all
  memset(&arguments, 0, sizeof(volumed_arguments));
  static struct option long_options[] = {
      {"debug", no_argument, 0, 'd'},
      {"foreground", no_argument, 0, 'f'},
      {"enable-abs-volume", no_argument, 0, 'A'},
      {"hotkey-code",  required_argument, 0, 'H'},
      {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "dfAH:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'd':
        arguments.debug = true;
        break;
      case 'H':
        arguments.hotkey_inputdevice_name = strtok(optarg, ":");
        arguments.hotkey_code = atoi(strtok(NULL, ":"));;
        break;
      case 'A':
        arguments.use_abs_volume = true;
        break;
      case 'f':
        arguments.foreground = true;
        break;
      default:
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  return true;
}
