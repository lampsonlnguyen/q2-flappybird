/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FLAPPY_AUDIO_H
#define FLAPPY_AUDIO_H
/* Events: 1 flap, 2 point, 3 collision, 4 start/restart. */
int audio_start(const char *app_path, const char *device, const char *volume);
void audio_play(int event);
void audio_stop(void);
#endif
