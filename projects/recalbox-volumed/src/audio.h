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

#ifndef RECALBOX_VOLUMED_AUDIO_H
#define RECALBOX_VOLUMED_AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <pulse/pulseaudio.h>

#define VOLUMED_CONTEXT_NAME "volumed"

typedef struct _volumed_struct_pa {
  pa_mainloop *mainloop;
  pa_mainloop_api *mainloop_api;
  pa_context *context;
} volumed_struct_pa;

volumed_struct_pa * initialize_audio();
void free_audio(volumed_struct_pa * struct_pa);
int init_context(volumed_struct_pa *struct_pa, int retval);

void wait_loop(volumed_struct_pa *pa, pa_operation *op);
pa_volume_t denormalize(int volume);
int constrain_volume(int volume);

// callbacks
static void cb_get_volume(pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol, void *userdata);
static void cb_get_default_sink_volume(pa_context *c, const pa_server_info *i, void *userdata);
static void cb_set_volume(pa_context *c, const pa_sink_info *i, __attribute__((unused)) int eol, void *userdata);
static void cb_get_server_info(__attribute__((unused)) pa_context *c, const pa_server_info *i, void *userdata);


void set_audio_volume(volumed_struct_pa *pa, int volume);
int get_audio_volume(volumed_struct_pa *p);

#endif
