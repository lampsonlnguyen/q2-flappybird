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
    assert(a.started && a.velocity256<0 && a.sound==4);
    a.velocity256=0;
    assert(!app_input(&a,"t u 185 160"));
    assert(!app_input(&a,"t m 185 160"));
    assert(a.velocity256==0);

    /* Centre is still immediate, without a second flap on release. */
    assert(app_input(&a,"k d 218"));
    assert(a.velocity256<0 && a.sound==1);
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
    assert(a.runner.status==Q2_GAME_RUNNING && a.started && a.sound==4);
    /* Game-over Back opens the same options box; Back again exits. */
    q2_game_over(&a.runner);
    a.dead=1;
    assert(!a.exit_menu);
    assert(app_input(&a,"k u 170"));
    assert(a.exit_menu && !a.quit && a.runner.status==Q2_GAME_OVER);
    assert(app_input(&a,"t d 185 160"));
    assert(!a.exit_menu && !a.dead && a.runner.status==Q2_GAME_RUNNING);
    q2_game_over(&a.runner);
    a.dead=1;
    assert(app_input(&a,"k u 170"));
    assert(a.exit_menu && !a.quit);
    assert(app_input(&a,"k u 170") && a.quit);
    puts("Q2 touch, Center, pause and game-over exit menu tests passed");
    return 0;
}
