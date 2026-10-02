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

#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>

#include "parse.h"
#include "log.h"

int _log_level;

void _openlog(const char * ident, int option, int facility, int log_level) {
  _log_level = log_level;
  openlog(ident, option, facility);
}

void _syslog(int priority, const char * format, ...) {
  if (priority <= _log_level) {
    va_list args;
    va_start(args, format);
    vsyslog(priority, format, args);
    if (arguments.foreground) {
      fprintf(stderr, "%s : ", priority_to_string(priority));
      vfprintf(stderr, format, args);
      fprintf(stderr, "\n");
    }
    va_end(args);
  }
}

void _closelog(void) {
  closelog();
}

const char *priority_to_string(int priority) {
    switch (priority & LOG_PRIMASK) {
        case LOG_EMERG:   return "EMERG  ";
        case LOG_ALERT:   return "ALERT  ";
        case LOG_CRIT:    return "CRIT   ";
        case LOG_ERR:     return "ERR    ";
        case LOG_WARNING: return "WARNING";
        case LOG_NOTICE:  return "NOTICE ";
        case LOG_INFO:    return "INFO   ";
        case LOG_DEBUG:   return "DEBUG  ";
        default:          return "UNKNOWN";
    }
}
