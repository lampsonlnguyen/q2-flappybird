/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    FlappyApp a;
    app_init(&a,42u,5u);
    assert(!a.started && !a.quit);
    assert(app_input(&a,"k d 218")==0);
    assert(app_input(&a,"k u 218")==1);
    assert(a.started && a.velocity256<0);
    assert(app_input(&a,"k u 171")==1);
    assert(a.runner.status==Q2_GAME_PAUSED);
    assert(app_input(&a,"k u 171")==1);
    assert(a.runner.status==Q2_GAME_RUNNING);
    q2_game_restart(&a.runner);
    assert(app_input(&a,"t u 180 160")==1);
    assert(a.started);
    q2_game_restart(&a.runner);
    assert(app_input(&a,"w -50")==1);
    assert(a.started);
    q2_game_restart(&a.runner);
    assert(app_input(&a,"k u 222")==1);
    assert(a.started);
    assert(app_input(&a,"k u 170")==1 && a.quit);
    puts("Q2 Flappy Bird input protocol tests passed");
    return 0;
}
