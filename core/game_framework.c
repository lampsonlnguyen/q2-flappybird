/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "game_framework.h"
void q2_game_init(Q2GameRunner *r, const Q2GameHooks *hooks, void *context,
                  unsigned step_ms)
{
    *r = (Q2GameRunner){0};
    r->hooks = hooks;
    r->context = context;
    r->step_ms = step_ms ? step_ms : 16;
    r->status = Q2_GAME_RUNNING;
    if (hooks && hooks->reset) hooks->reset(context);
}
void q2_game_pause(Q2GameRunner *r)
{
    if (r->status == Q2_GAME_RUNNING) r->status = Q2_GAME_PAUSED;
    r->remainder_ms = 0;
    r->touch_active = 0;
}
void q2_game_resume(Q2GameRunner *r)
{
    if (r->status == Q2_GAME_PAUSED) r->status = Q2_GAME_RUNNING;
    r->remainder_ms = 0;
}
void q2_game_over(Q2GameRunner *r)
{
    r->status = Q2_GAME_OVER;
    r->remainder_ms = 0;
    r->touch_active = 0;
}
void q2_game_restart(Q2GameRunner *r)
{
    if (r->hooks && r->hooks->reset) r->hooks->reset(r->context);
    r->status = Q2_GAME_RUNNING;
    r->remainder_ms = 0;
    r->touch_active = 0;
}
int q2_game_input(Q2GameRunner *r, Q2GameInput ev)
{
    if (ev.action == Q2_GAME_PLAY) {
        if (r->status == Q2_GAME_PAUSED) q2_game_resume(r);
        else if (r->status == Q2_GAME_RUNNING) q2_game_pause(r);
        return 1;
    }
    if (r->status != Q2_GAME_RUNNING) return 0;
    if (ev.action == Q2_GAME_TOUCH_DOWN) r->touch_active = 1;
    else if (ev.action == Q2_GAME_TOUCH_UP) r->touch_active = 0;
    else if (ev.action == Q2_GAME_TOUCH_MOVE && !r->touch_active) return 0;
    return r->hooks && r->hooks->input ? !!r->hooks->input(r->context, ev) : 0;
}
unsigned q2_game_advance(Q2GameRunner *r, unsigned elapsed_ms)
{
    if (r->status != Q2_GAME_RUNNING || !r->hooks || !r->hooks->step) return 0;
    unsigned cap = r->step_ms * 4;
    if (elapsed_ms > cap) elapsed_ms = cap;
    r->remainder_ms += elapsed_ms;
    unsigned ticks = 0;
    while (r->remainder_ms >= r->step_ms && ticks < 4 &&
           r->status == Q2_GAME_RUNNING) {
        r->remainder_ms -= r->step_ms;
        r->hooks->step(r->context, r->step_ms);
        ++ticks;
    }
    return ticks;
}
void q2_game_paint(const Q2GameRunner *r, void *canvas)
{
    if (r->hooks && r->hooks->paint) r->hooks->paint(r->context, canvas);
}

int q2_game_ui_safe_point(int width, int height, int x, int y)
{
    if (width <= 0 || height <= 0 || x < 0 || y < 0 ||
        x >= width || y >= height) return 0;

    /* Keep UI text off the glass edges. Rounded corners require a wider
     * horizontal inset in the top/bottom 42 pixels. */
    int edge = width / 24;
    if (edge < 12) edge = 12;
    int corner = width / 8;
    if (corner < edge) corner = edge;
    int band = height / 8;
    if (band < 24) band = 24;
    int vertical = height / 32;
    if (vertical < 8) vertical = 8;
    if (y < vertical || y >= height - vertical) return 0;
    int margin = y < band || y >= height - band ? corner : edge;
    return x >= margin && x < width - margin;
}
