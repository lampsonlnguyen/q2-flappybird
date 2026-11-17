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
    a->ticks=0;
    a->spawn_ticks=0;
    for(int i=0;i<PIPE_COUNT;++i) {
        a->pipes[i].x=SCREEN_W+50+i*172;
        a->pipes[i].gap_y=92+(int)(random_next(a)%82u);
        a->pipes[i].passed=0;
    }
}
static void flap(FlappyApp *a) {
    a->started=1;
    a->velocity256=-5*256;
}
/* Match Dino Runner: centre key release and touchscreen press flap.
 * Wheel rotation and touch-release packets must not flap. */
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
            if(a->score>a->best) { a->best=a->score; a->dirty=1; }
        }
        if(BIRD_X+BIRD_RADIUS-3>p->x && BIRD_X-BIRD_RADIUS+3<p->x+PIPE_WIDTH &&
           (top<p->gap_y || bottom>p->gap_y+PIPE_GAP)) a->dead=1;
    }
    if(a->dead) q2_game_over(&a->runner);
}
static const Q2GameHooks hooks={game_input,game_step,0,game_reset};
void app_init(FlappyApp *a,unsigned seed,unsigned best) {
    *a=(FlappyApp){0};
    a->seed=(int)(seed?seed:1u);
    a->best=(int)best;
    q2_game_init(&a->runner,&hooks,a,17);
}
static int parse_int(const char *s,int *out) {
    int neg=0,n=0,found=0;
    while(*s==' ')++s;
    if(*s=='-'){neg=1;++s;}
    while(*s>='0'&&*s<='9'){
        if(n>100000)return 0;
        n=n*10+*s++-'0';found=1;
    }
    if(!found)return 0;
    *out=neg?-n:n;
    return 1;
}
static int play_action(FlappyApp *a,Q2GameAction action,int x,int y) {
    if(a->runner.status==Q2_GAME_PAUSED) q2_game_resume(&a->runner);
    if(a->runner.status==Q2_GAME_OVER) q2_game_restart(&a->runner);
    return q2_game_input(&a->runner,(Q2GameInput){action,0,x,y});
}
int app_input(FlappyApp *a,const char *message) {
    /* Matches q2-dino-runner: k u 218 for centre, t d X Y for touch.
     * Ignore wheel movement, touch release, and repeated touch movement. */
    if(!message || !message[0] || message[1]!=' ' ||
       !message[2] || message[3]!=' ') return 0;
    if(message[0]=='k' && message[2]=='u') {
        int key=0;
        if(!parse_int(message+4,&key)) return 0;
        if(key==KEY_RETURN) {
            if(a->runner.status==Q2_GAME_PAUSED) a->quit=1;
            else q2_game_pause(&a->runner);
            return 1;
        }
        if(key==KEY_CENTER) return play_action(a,Q2_GAME_CENTER,0,0);
    }
    if(message[0]=='t' && message[2]=='d') {
        const char *p=message+4;
        int x=0,y=0;
        if(!parse_int(p,&x)) return 0;
        while(*p==' ') ++p;
        if(*p=='-') ++p;
        while(*p>='0' && *p<='9') ++p;
        if(*p!=' ' || !parse_int(p,&y) ||
           x<0 || x>=SCREEN_W || y<0 || y>=SCREEN_H) return 0;
        return play_action(a,Q2_GAME_TOUCH_DOWN,x,y);
    }
    return 0;
}
int app_animated(const FlappyApp *a){return a->runner.status==Q2_GAME_RUNNING;}
int app_tick(FlappyApp *a,unsigned elapsed_ms) {
    return (int)q2_game_advance(&a->runner,elapsed_ms);
}
