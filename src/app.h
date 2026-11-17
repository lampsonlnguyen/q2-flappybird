/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef Q2_FLAPPY_APP_H
#define Q2_FLAPPY_APP_H
#include <stdint.h>
#include "game_framework.h"
enum { SCREEN_W=375, SCREEN_H=320, GROUND_Y=283, BIRD_X=112,
       BIRD_RADIUS=10, PIPE_WIDTH=48, PIPE_GAP=110, PIPE_COUNT=3 };
typedef uint32_t u32;
typedef struct { int x, gap_y, passed; } FlappyPipe;
typedef struct {
    Q2GameRunner runner;
    int y256, velocity256, started, dead, score, best, dirty, quit;
    int ticks, seed, spawn_ticks;
    FlappyPipe pipes[PIPE_COUNT];
} FlappyApp;
void app_init(FlappyApp *app, unsigned seed, unsigned best);
int app_input(FlappyApp *app, const char *message);
int app_tick(FlappyApp *app, unsigned elapsed_ms);
int app_animated(const FlappyApp *app);
void app_render(const FlappyApp *app, u32 *frame, unsigned stride);
#endif
