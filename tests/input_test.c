/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include <assert.h>
#include <stdio.h>
static void clear_motion(FlappyApp *a) { a->velocity256=0; }
int main(void) {
    FlappyApp a;
    app_init(&a,42u,5u);
    assert(!a.started && !a.quit);
    assert(!app_input(&a,"w -50"));
    assert(!app_input(&a,"k u 222"));
    assert(!app_input(&a,"t d -1 160"));
    assert(!app_input(&a,"t d 375 160"));
    assert(!app_input(&a,"t m 185 320"));

    /* Center responds on physical button down, without waiting for release. */
    assert(app_input(&a,"k d 218"));
    assert(a.started && a.velocity256<0 && a.y256<143*256);
    clear_motion(&a);
    assert(!app_input(&a,"k d 218"));
    assert(!app_input(&a,"k u 218"));
    assert(a.velocity256==0);
    assert(app_input(&a,"k d 218"));
    assert(a.velocity256<0);
    assert(!app_input(&a,"k u 218"));
    clear_motion(&a);
    assert(app_input(&a,"k u 218")); /* release-only device fallback */

    /* First contact flaps immediately; movement/release do not double-flap. */
    clear_motion(&a);
    assert(app_input(&a,"t d 185 160"));
    assert(a.velocity256<0);
    clear_motion(&a);
    assert(!app_input(&a,"t m 188 164"));
    assert(!app_input(&a,"t d 188 164"));
    assert(!app_input(&a,"t u 188 164"));
    assert(a.velocity256==0);

    /* A move-first gesture and a release-only gesture both work. */
    assert(app_input(&a,"t m 185 160"));
    assert(a.velocity256<0);
    clear_motion(&a);
    assert(!app_input(&a,"t m 190 160"));
    assert(!app_input(&a,"t u -1 -1")); /* abort/out-of-range release */
    assert(a.velocity256==0);
    assert(app_input(&a,"t u -1 -1"));
    assert(a.velocity256<0);

    /* Resume and restart on first touch contact. */
    assert(app_input(&a,"k u 170"));
    assert(a.runner.status==Q2_GAME_PAUSED);
    assert(app_input(&a,"t d 185 160"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.velocity256<0);
    assert(!app_input(&a,"t u 185 160"));
    q2_game_over(&a.runner);
    assert(app_input(&a,"t d 185 160"));
    assert(a.runner.status==Q2_GAME_RUNNING && a.started && a.velocity256<0);
    assert(!app_input(&a,"t u 185 160"));
    assert(app_input(&a,"k u 170"));
    assert(app_input(&a,"k u 170") && a.quit);
    puts("Q2 immediate centre/touch-down/move/release tests passed");
    return 0;
}
