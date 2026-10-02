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


#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <pulse/pulseaudio.h>

#include "audio.h"
#include "log.h"
#include "volumed.h"

int retval;

void free_audio(volumed_struct_pa * struct_pa) {
  if (struct_pa) {
    if (struct_pa->context) {
        pa_context_unref(struct_pa->context);
    }
    if (struct_pa->mainloop_api) {
        struct_pa->mainloop_api->quit(struct_pa->mainloop_api, retval);
    }
    if (struct_pa->mainloop) {
        pa_signal_done();
        pa_mainloop_free(struct_pa->mainloop);
    }
    free(struct_pa);
  }
}

volumed_struct_pa * initialize_audio() {
  // initialize pulseaudio
  volumed_struct_pa * struct_pa = calloc(1, sizeof(volumed_struct_pa));
  if (!struct_pa)
    return NULL;
  struct_pa->mainloop = pa_mainloop_new();
  if (!struct_pa->mainloop) {
      _syslog(LOG_ERR, "Could not create PulseAudio main loop\n");
      free_audio(struct_pa);
      return NULL;
  }

  struct_pa->mainloop_api = pa_mainloop_get_api(struct_pa->mainloop);
  if (pa_signal_init(struct_pa->mainloop_api) != 0) {
      _syslog(LOG_ERR, "Could not initialize PulseAudio UNIX signal subsystem\n");
      free_audio(struct_pa);
      return NULL;
  }

  struct_pa->context = pa_context_new(struct_pa->mainloop_api, VOLUMED_CONTEXT_NAME);
  if (!struct_pa->context || init_context(struct_pa, retval) != 0) {
      _syslog(LOG_ERR, "Could not initialize PulseAudio context\n");
      free_audio(struct_pa);
      return NULL;
  }
  return struct_pa;
}

void wait_loop(volumed_struct_pa *pa, pa_operation *op) {
  if (op) {
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
      if (pa_mainloop_iterate(pa->mainloop, 1, &retval) < 0) {
        break;
      }
    }
    pa_operation_unref(op);
  }
}

// % to pa_volume_t
pa_volume_t denormalize(int volume) {
    return (pa_volume_t) rintf(volume * PA_VOLUME_NORM / 100.0);
}

// pa_volume_t to %
int normalize(pa_volume_t volume) {
    return (int) rintf(volume * 100.0 / PA_VOLUME_NORM);
}

int constrain_volume(int volume) {
    if (volume > 100) {
        return 100;
    }
    if (volume < 0) {
        return 0;
    }
    return volume;
}

int init_context(volumed_struct_pa * pa, int retval) {
    pa_context_connect(pa->context, NULL, PA_CONTEXT_NOFLAGS, NULL);
    pa_context_state_t state;
    while (state = pa_context_get_state(pa->context), true) {
        if (state == PA_CONTEXT_READY) {
            return 0;
        }
        if (state == PA_CONTEXT_FAILED) {
            return 1;
        }
        pa_mainloop_iterate(pa->mainloop, 1, &retval);
    }
}

// callbacks
static void cb_set_volume(pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol, void *userdata) {
    if (i == NULL) {
        return;
    }
    int *volume = (int*)userdata;
    pa_cvolume *cvolume = (pa_cvolume*)&i->volume;
    const pa_cvolume *new_cvolume = pa_cvolume_set(cvolume, i->volume.channels, denormalize(constrain_volume(*volume)));
    pa_operation *op = pa_context_set_sink_volume_by_index(c, i->index, new_cvolume, NULL, NULL);
    if (op) pa_operation_unref(op);
    _syslog(LOG_DEBUG, "cb_set_volume(%d)=%d", *volume, denormalize(constrain_volume(*volume)));
}

static void cb_get_volume(pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol, void *userdata) {
    if (i == NULL) {
        return;
    }
    int *volume = (int*)userdata;

    *volume = normalize(pa_cvolume_avg((pa_cvolume*) &i->volume));
    _syslog(LOG_DEBUG, "cb_get_volume()=%d", *volume);
}

static void cb_get_default_sink_volume(pa_context *c, const pa_server_info *i, void *userdata) {
    if (i == NULL) {
        return;
    }
    pa_operation *op = pa_context_get_sink_info_by_name(c, i->default_sink_name, cb_get_volume, userdata);
    if (op) pa_operation_unref(op);
}

static void cb_get_server_info(__attribute__((unused)) pa_context *c, const pa_server_info *i, void *userdata) {
    if (i == NULL) {
        return;
    }
    strncpy(userdata, i->default_sink_name, 255);
}

void set_audio_volume(volumed_struct_pa *pa, int volume) {
  char default_sink_name[256];
  wait_loop(pa, pa_context_get_server_info(pa->context, cb_get_server_info, &default_sink_name));
  wait_loop(pa, pa_context_get_sink_info_by_name(pa->context, (char *) default_sink_name, cb_set_volume, &volume));
}

int get_audio_volume(volumed_struct_pa * pa) {
  int volume = 0;
  wait_loop(pa, pa_context_get_server_info(pa->context, cb_get_default_sink_volume, &volume));
  return volume;
}

