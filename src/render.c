/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 375x320 rotated framebuffer. Original pixel art; no borrowed assets.
 */
#include "app.h"
#include "font.h"
static void fill(u32 *d,unsigned s,int x,int y,int w,int h,u32 c) {
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>SCREEN_W)w=SCREEN_W-x;
    if(y+h>SCREEN_H)h=SCREEN_H-y;
    if(w<=0||h<=0)return;
    c|=0xff000000u;
    for(int xx=x;xx<x+w;++xx)
        for(int yy=y;yy<y+h;++yy)d[xx*s+SCREEN_H-1-yy]=c;
}
static void circle(u32 *d,unsigned s,int cx,int cy,int r,u32 c) {
    for(int y=-r;y<=r;++y)
        for(int x=-r;x<=r;++x)if(x*x+y*y<=r*r)fill(d,s,cx+x,cy+y,1,1,c);
}
static int width(const char *str,int scale){
    const unsigned char *adv=scale==1?f12_adv:f18_adv;
    int n=0;
    while(*str){unsigned ch=(unsigned char)*str++;
        if(ch>='a'&&ch<='z')ch-=32;
        n+=adv[ch>=32&&ch<=90?ch-32:0];
    }return n;
}
static void text(u32 *d,unsigned s,int x,int y,const char *str,u32 color,int scale){
    const unsigned char *adv=scale==1?f12_adv:f18_adv;
    const unsigned short *off=scale==1?f12_off:f18_off;
    const unsigned char *px=scale==1?f12_px:f18_px;
    int height=scale==1?12:17;
    while(*str){
        unsigned ch=(unsigned char)*str++;
        if(ch>='a'&&ch<='z')ch-=32;
        unsigned idx=ch>=32&&ch<=90?ch-32:0;
        int w=adv[idx];
        for(int yy=0;yy<height;++yy)for(int xx=0;xx<w;++xx){
            unsigned alpha=px[off[idx]+yy*w+xx];
            if(alpha<80 || !q2_game_ui_safe_point(SCREEN_W,SCREEN_H,x+xx,y+yy))continue;
            fill(d,s,x+xx,y+yy,1,1,color);
        }
        x+=w;
    }
}
static void centered(u32*d,unsigned s,int y,const char*str,u32 color,int scale){
    text(d,s,(SCREEN_W-width(str,scale))/2,y,str,color,scale);
}
static void number(u32*d,unsigned s,int y,unsigned value,u32 color) {
    char tmp[12];int n=0;
    do {tmp[n++]=(char)('0'+value%10u);value/=10u;}while(value&&n<10);
    char str[12];
    for(int i=0;i<n;++i)str[i]=tmp[n-i-1];
    str[n]=0;centered(d,s,y,str,color,3);
}
void app_render(const FlappyApp *a,u32 *d,unsigned s){
    const u32 sky=0x81d8e5,cloud=0xe6fcf5,pipe=0x55bd4b,
        shadow=0x26783f,grass=0x84c75d,dirt=0xe8d49b,ink=0x24313a;
    fill(d,s,0,0,SCREEN_W,SCREEN_H,sky);
    for(int i=0;i<5;++i){
        int x=(i*113+SCREEN_W-(a->ticks/3)%SCREEN_W)%SCREEN_W;
        circle(d,s,x,65+(i%3)*37,17,cloud);
        circle(d,s,x+16,70+(i%3)*37,13,cloud);
        fill(d,s,x-18,71+(i%3)*37,48,8,cloud);
    }
    for(int i=0;i<PIPE_COUNT;++i){
        const FlappyPipe *p=&a->pipes[i];int x=p->x;
        fill(d,s,x,0,PIPE_WIDTH,p->gap_y,shadow);
        fill(d,s,x+5,0,PIPE_WIDTH-10,p->gap_y,pipe);
        fill(d,s,x-4,p->gap_y-18,PIPE_WIDTH+8,18,shadow);
        fill(d,s,x,p->gap_y-15,PIPE_WIDTH,12,pipe);
        int lower=p->gap_y+PIPE_GAP;
        fill(d,s,x,lower,PIPE_WIDTH,GROUND_Y-lower,shadow);
        fill(d,s,x+5,lower,PIPE_WIDTH-10,GROUND_Y-lower,pipe);
        fill(d,s,x-4,lower,PIPE_WIDTH+8,18,shadow);
        fill(d,s,x,lower+3,PIPE_WIDTH,12,pipe);
    }
    int by=a->y256/256;
    circle(d,s,BIRD_X,by,BIRD_RADIUS+2,0x8d612e);
    circle(d,s,BIRD_X,by,BIRD_RADIUS,0xffdc53);
    circle(d,s,BIRD_X-3,by+4,5,0xf8b741);
    fill(d,s,BIRD_X+6,by-2,11,5,0xf19531);
    circle(d,s,BIRD_X+3,by-5,4,0xffffff);
    circle(d,s,BIRD_X+4,by-5,2,ink);
    fill(d,s,0,GROUND_Y,SCREEN_W,6,grass);
    fill(d,s,0,GROUND_Y+6,SCREEN_W,SCREEN_H-GROUND_Y-6,dirt);
    for(int i=0;i<20;++i)fill(d,s,i*23-(a->ticks%23),294,12,3,0xc3ae79);
    if(a->started && !a->dead)number(d,s,18,(unsigned)a->score,0xffffff);
    if(!a->started){
        centered(d,s,46,"FLAPPY BIRD",ink,3);
        centered(d,s,194,"CENTER TO FLAP",ink,3);
        centered(d,s,223,"AVOID THE PIPES",ink,1);
    }
    if(a->runner.status==Q2_GAME_PAUSED || a->exit_menu){
        fill(d,s,59,96,257,113,0xf9f1c8);
        centered(d,s,115,a->exit_menu?"GAME OVER":"PAUSED",ink,3);
        centered(d,s,153,a->exit_menu?"CENTER TO RETRY":"CENTER TO RESUME",ink,1);
        centered(d,s,172,"BACK TO EXIT",ink,1);
    } else if(a->dead){
        fill(d,s,57,78,261,148,0xf9f1c8);
        centered(d,s,91,"GAME OVER",ink,3);
        centered(d,s,119,"SCORE",ink,1);
        number(d,s,135,(unsigned)a->score,ink);
        centered(d,s,166,"BEST",ink,1);
        number(d,s,179,(unsigned)a->best,ink);
        centered(d,s,207,"CENTER RETRY",ink,1);
    }
}
