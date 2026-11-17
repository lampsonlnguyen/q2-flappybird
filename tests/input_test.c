/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    FlappyApp a;
    app_init(&a,42u,5u);
    assert(!a.started && !a.quit);
    /* Click wheel is NOT a touchscreen or flap trigger. */
    assert(!app_input(&a,"w -50"));
    assert(!app_input(&a,"w 50"));
    assert(!app_input(&a,"t d 185 160"));
    assert(!app_input(&a,"t u 185 160"));
    assert(!app_input(&a,"k u 222"));
    assert(!app_input(&a,"k u 223"));
    assert(!app_input(&a,"k d 218"));
    assert(!a.started);
    assert(app_input(&a,"k u 218"));
    assert(a.started && a.velocity256 < 0);
    a.velocity256=0;
    assert(!app_input(&a,"t u 185 160"));
    assert(a.velocity256==0);
    assert(app_input(&a,"k u 170"));
    assert(a.runner.status==Q2_GAME_PAUSED && !a.quit);
    assert(app_input(&a,"k u 218"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.velocity256==0);
    q2_game_over(&a.runner);
    assert(app_input(&a,"k u 218"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.started && a.velocity256<0);
    assert(app_input(&a,"k u 170"));
    assert(a.runner.status==Q2_GAME_PAUSED);
    assert(app_input(&a,"k u 170") && a.quit);
    puts("Q2 Dino-style centre/return input tests passed");
    return 0;
}
