/* SPDX-License-Identifier: GPL-3.0-or-later
 * Flappy Bird mechanics reimplemented for Q2's shared game framework.
 * Fixed-point physics, no heap, SDL, or device-specific game logic.
 */
#include "app.h"
enum { KEY_RETURN=170, KEY_CENTER=218 };
static unsigned random_next(FlappyApp *a) {
    unsigned v=(unsigned)a->seed;
    v=v*1664525u+1013904223u;
    a->seed=(int)v;
    return v;
}
static void game_reset(void *context) {
    FlappyApp *a=(FlappyApp *)context;
    a->y256=143*256;
    a->velocity256=0;
    a->score=0;
    a->started=0;
    a->dead=0;
    a->exit_menu=0;
    a->ticks=0;
    a->spawn_ticks=0;
    a->center_down=0;
    for(int i=0;i<PIPE_COUNT;++i) {
        a->pipes[i].x=SCREEN_W+50+i*172;
        a->pipes[i].gap_y=92+(int)(random_next(a)%82u);
        a->pipes[i].passed=0;
    }
}
static void flap(FlappyApp *a) {
    a->started=1;
    a->velocity256=-5*256;
    /* Show the flap immediately, without waiting for a physics tick. */
    a->y256-=2*256;
    a->sound=1; /* flap */
}
/* Centre key presses and first touchscreen contact flap immediately. */
static int game_input(void *context,Q2GameInput event) {
    FlappyApp *a=(FlappyApp *)context;
    if(event.action!=Q2_GAME_CENTER && event.action!=Q2_GAME_TOUCH_DOWN) return 0;
    flap(a);
    return 1;
}
static void game_step(void *context,unsigned ms) {
    FlappyApp *a=(FlappyApp *)context;
    (void)ms;
    ++a->ticks;
    if(!a->started || a->dead) return;
    /* 60 Hz fixed-point gravity and terminal velocity. */
    a->velocity256+=76;
    if(a->velocity256>6*256) a->velocity256=6*256;
    a->y256+=a->velocity256;
    int top=a->y256/256-BIRD_RADIUS+2;
    int bottom=a->y256/256+BIRD_RADIUS-2;
    if(top<5 || bottom>=GROUND_Y) a->dead=1;
    for(int i=0;i<PIPE_COUNT;++i) {
        FlappyPipe *p=&a->pipes[i];
        p->x-=2;
        if(p->x+PIPE_WIDTH<0) {
            int furthest=0;
            for(int j=0;j<PIPE_COUNT;++j)
                if(a->pipes[j].x>furthest) furthest=a->pipes[j].x;
            p->x=furthest+172;
            p->gap_y=88+(int)(random_next(a)%86u);
            p->passed=0;
        }
        if(!p->passed && p->x+PIPE_WIDTH<BIRD_X-BIRD_RADIUS) {
            p->passed=1;
            ++a->score;
            a->sound=2; /* passed pipe */
            if(a->score>a->best) { a->best=a->score; a->dirty=1; }
        }
        if(BIRD_X+BIRD_RADIUS-3>p->x && BIRD_X-BIRD_RADIUS+3<p->x+PIPE_WIDTH &&
           (top<p->gap_y || bottom>p->gap_y+PIPE_GAP)) a->dead=1;
    }
    if(a->dead) {
        a->sound=3; /* collision overrides point */
        q2_game_over(&a->runner);
    }
}
static const Q2GameHooks hooks={game_input,game_step,0,game_reset};
void app_init(FlappyApp *a,unsigned seed,unsigned best) {
    *a=(FlappyApp){0};
    a->seed=(int)(seed?seed:1u);
    a->best=(int)best;
    q2_game_init(&a->runner,&hooks,a,17);
}
/* Same coordinate parser as q2-dino-runner/src/app.c. */
static int decimal(const char **message) {
    const char *p=*message;
    while(*p==' ') ++p;
    if(*p<'0'||*p>'9') return -1;
    int value=0;
    while(*p>='0'&&*p<='9'&&value<1000)
        value=value*10+*p++-'0';
    *message=p;
    return *p>='0'&&*p<='9' ? -1 : value;
}
static int play_action(FlappyApp *a,Q2GameAction action,int x,int y) {
    int starting=!a->started || a->runner.status==Q2_GAME_OVER;
    a->exit_menu=0;
    if(a->runner.status==Q2_GAME_PAUSED) q2_game_resume(&a->runner);
    if(a->runner.status==Q2_GAME_OVER) q2_game_restart(&a->runner);
    int handled=q2_game_input(&a->runner,(Q2GameInput){action,0,x,y});
    if(handled && starting) a->sound=4; /* start/restart */
    return handled;
}
int app_input(FlappyApp *a,const char *message) {
    if(!message || !message[0] || message[1]!=' ' ||
       !message[2] || message[3]!=' ') return 0;
    const char *args=message+4;
    int value=decimal(&args);
    if(message[0]=='k') {
        if(value<0) return 0;
        if(value==KEY_RETURN && message[2]=='u') {
            if(a->runner.status==Q2_GAME_PAUSED || a->exit_menu)
                a->quit=1;
            else if(a->runner.status==Q2_GAME_OVER)
                a->exit_menu=1;
            else
                q2_game_pause(&a->runner);
            return 1;
        }
        if(value!=KEY_CENTER) return 0;
        if(message[2]=='d') {
            if(a->center_down) return 0;
            int handled=play_action(a,Q2_GAME_CENTER,0,0);
            a->center_down=1;
            return handled;
        }
        if(message[2]=='u') {
            if(a->center_down) {
                a->center_down=0;
                return 0;
            }
            return play_action(a,Q2_GAME_CENTER,0,0);
        }
        return 0;
    }
    /* Dino Runner's exact touch-down parser: act when the panel is pressed,
     * not on pointer movement or release. */
    if(message[0]!='t' || message[2]!='d') return 0;
    int y=decimal(&args);
    if(value<0 || value>=SCREEN_W || y<0 || y>=SCREEN_H)
        return 0;
    return play_action(a,Q2_GAME_TOUCH_DOWN,value,y);
}
int app_animated(const FlappyApp *a){return a->runner.status==Q2_GAME_RUNNING;}
int app_tick(FlappyApp *a,unsigned elapsed_ms) {
    return (int)q2_game_advance(&a->runner,elapsed_ms);
}
