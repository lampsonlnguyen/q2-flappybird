/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    FlappyApp a;
    app_init(&a,42u,5u);
    assert(!a.started);
    assert(!app_input(&a,"w -50"));
    assert(!app_input(&a,"t m 185 160"));
    assert(!app_input(&a,"k u 222"));
    assert(!app_input(&a,"t d -1 160"));
    assert(!app_input(&a,"t d 375 160"));

    /* Initial hardware press must flap without waiting for release. */
    assert(app_input(&a,"k d 218"));
    assert(a.started && a.velocity256<0 && a.y256<143*256);
    a.velocity256=0;
    assert(!app_input(&a,"k d 218")); /* held/repeat ignored */
    assert(!app_input(&a,"k u 218")); /* release never adds another flap */
    assert(a.velocity256==0);
    assert(app_input(&a,"k d 218"));
    assert(a.velocity256<0);
    assert(!app_input(&a,"k u 218"));

    /* A release-only firmware still works. */
    a.velocity256=0;
    assert(app_input(&a,"k u 218"));
    assert(a.velocity256<0);

    a.velocity256=0;
    assert(app_input(&a,"t d 185 160"));
    assert(a.velocity256<0);
    a.velocity256=0;
    assert(!app_input(&a,"t d 185 160"));
    assert(a.velocity256==0);
    assert(!app_input(&a,"t u 185 160"));
    assert(a.velocity256==0);
    assert(app_input(&a,"t u 185 160")); /* release-only fallback */

    /* Pause/resume and restart must not double-flap. */
    assert(app_input(&a,"k u 170"));
    assert(a.runner.status==Q2_GAME_PAUSED);
    assert(app_input(&a,"k d 218"));
    assert(a.runner.status==Q2_GAME_RUNNING);
    assert(!app_input(&a,"k u 218"));
    q2_game_over(&a.runner);
    assert(app_input(&a,"k d 218"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.started);
    assert(!app_input(&a,"k u 218"));
    assert(app_input(&a,"k u 170"));
    assert(app_input(&a,"k u 170") && a.quit);
    puts("Low-latency Q2 centre and touch controls passed");
    return 0;
}
