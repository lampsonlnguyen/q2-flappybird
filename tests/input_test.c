/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    FlappyApp a;
    app_init(&a,42u,5u);
    assert(!a.started);
    assert(!app_input(&a,"w 30"));

    /* Same touch-down format and parsing as Dino Runner. */
    assert(!app_input(&a,"t m 185 160"));
    assert(!app_input(&a,"t u 185 160"));
    assert(!app_input(&a,"t d -1 160"));
    assert(!app_input(&a,"t d 375 160"));
    assert(!app_input(&a,"t d 185 320"));
    assert(app_input(&a,"t d 185 160"));
    assert(a.started && a.velocity256<0);
    a.velocity256=0;
    assert(!app_input(&a,"t u 185 160"));
    assert(!app_input(&a,"t m 185 160"));
    assert(a.velocity256==0);

    /* Centre is still immediate, without a second flap on release. */
    assert(app_input(&a,"k d 218"));
    assert(a.velocity256<0);
    a.velocity256=0;
    assert(!app_input(&a,"k d 218"));
    assert(!app_input(&a,"k u 218"));
    assert(a.velocity256==0);
    assert(app_input(&a,"k d 218"));
    assert(!app_input(&a,"k u 218"));

    assert(app_input(&a,"k u 170"));
    assert(a.runner.status==Q2_GAME_PAUSED);
    assert(app_input(&a,"t d 185 160"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.velocity256<0);
    q2_game_over(&a.runner);
    assert(app_input(&a,"t d 185 160"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.started);
    assert(app_input(&a,"k u 170"));
    assert(app_input(&a,"k u 170") && a.quit);
    puts("Dino Runner touch-down and immediate Centre controls passed");
    return 0;
}
