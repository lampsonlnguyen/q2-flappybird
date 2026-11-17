/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 card runtime using the Linux/MIPS system-call ABI and framebuffer contract.
 * Startup/syscall pattern adapted from DiamondBond's Buddy/q2-fidget card.
 */
#include "app.h"
long sc(long n,long a,long b,long c,long d,long e,long f);
__asm__(".text\n.set noreorder\n.globl sc\nsc:\n"
        "move $2, $4\nmove $4, $5\nmove $5, $6\nmove $6, $7\n"
        "lw $7, 16($29)\nlw $8, 20($29)\nlw $9, 24($29)\n"
        "addiu $29, $29, -32\nsw $8, 16($29)\nsw $9, 20($29)\n"
        "syscall\naddiu $29, $29, 32\n"
        "beqz $7, 1f\nnop\nsubu $2, $0, $2\n1: jr $31\nnop\n"
        ".globl __start\n__start:\n"
        "lw $4, 0($29)\naddiu $5, $29, 4\nli $8, -8\nand $29, $29, $8\n"
        "addiu $29, $29, -32\njal q2_main\nnop\nmove $4, $2\nli $2, 4246\nsyscall\n1: b 1b\nnop\n.set reorder\n");

enum { SYS_READ=4003,SYS_WRITE=4004,SYS_OPEN=4005,SYS_CLOSE=4006,
       SYS_UNLINK=4010,SYS_RENAME=4038,SYS_IOCTL=4054,SYS_MMAP=4090,
       SYS_MUNMAP=4091,SYS_FSYNC=4118,SYS_BIND=4169,SYS_RECV=4175,
       SYS_SOCKET=4183,SYS_POLL=4188,SYS_CLOCK_GETTIME=4263,
       FB_VSCREEN=0x4600,FB_FSCREEN=0x4602,FB_PAN=0x4606,
       O_RDONLY=0,O_WRONLY=1,O_RDWR=2,O_CREAT=0x100,O_TRUNC=0x200,O_CLOEXEC=0x80000,
       MSG_DONTWAIT=0x40,FRAME_MS=17 };
#define INPUT_SOCKET "/tmp/q2-flappybird.input.sock"
#define SAVE_FILE "/mnt/mmc/.flappybird/highscores"
#define SAVE_TEMP "/mnt/mmc/.flappybird/highscores.new"
#define SAVE_MAGIC 0x51324642u
#define DEBUG_FLAG "/mnt/mmc/.flappybird/debug-input"
#define DEBUG_LOG "/mnt/mmc/.flappybird/input.log"
static FlappyApp app;

void *memset(void *dest,int value,__SIZE_TYPE__ length) {
    unsigned char *p=dest;
    while(length--)*p++=(unsigned char)value;
    return dest;
}
void *memcpy(void *dest,const void *src,__SIZE_TYPE__ length) {
    unsigned char *p=dest; const unsigned char *q=src;
    while(length--)*p++=*q++;
    return dest;
}
static unsigned now_ms(void) {
    long time[2]={0};
    sc(SYS_CLOCK_GETTIME,1,(long)time,0,0,0,0);
    return (unsigned)time[0]*1000u+(unsigned)time[1]/1000000u;
}
struct save {u32 magic,best,checksum;};
static struct save saved;
static void load(void) {
    struct save file;
    int fd=(int)sc(SYS_OPEN,(long)SAVE_FILE,O_CLOEXEC,0,0,0,0);
    if(fd<0)return;
    long n=sc(SYS_READ,fd,(long)&file,sizeof file,0,0,0);
    sc(SYS_CLOSE,fd,0,0,0,0,0);
    if(n==(long)sizeof file && file.magic==SAVE_MAGIC &&
       file.checksum==(SAVE_MAGIC^file.best^0xa5a55a5au))saved=file;
}
static void save(void) {
    struct save file={SAVE_MAGIC,(u32)app.best,0};
    file.checksum=SAVE_MAGIC^file.best^0xa5a55a5au;
    int fd=(int)sc(SYS_OPEN,(long)SAVE_TEMP,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0644,0,0,0);
    if(fd<0)return;
    int good=sc(SYS_WRITE,fd,(long)&file,sizeof file,0,0,0)==(long)sizeof file &&
             sc(SYS_FSYNC,fd,0,0,0,0,0)==0;
    if(sc(SYS_CLOSE,fd,0,0,0,0,0))good=0;
    if(good && !sc(SYS_RENAME,(long)SAVE_TEMP,(long)SAVE_FILE,0,0,0,0)){app.dirty=0;return;}
    sc(SYS_UNLINK,(long)SAVE_TEMP,0,0,0,0,0);
}
/* Optional hardware input trace: a marker on the SD card enables logging.
 * Never runs during ordinary gameplay. Raw datagrams precede app_input(). */
static int debug_fd=-1;
static unsigned debug_count;
static void debug_begin(void) {
    int flag=(int)sc(SYS_OPEN,(long)DEBUG_FLAG,O_RDONLY|O_CLOEXEC,0,0,0,0);
    if(flag<0)return;
    sc(SYS_CLOSE,flag,0,0,0,0,0);
    debug_fd=(int)sc(SYS_OPEN,(long)DEBUG_LOG,
        O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0644,0,0,0);
    if(debug_fd>=0) {
        static const char header[]=
            "Q2 Flappy Bird raw input trace (ms raw accepted status y256)\\n"
            "status: 0=running 1=paused 2=game-over; max 1000 events\\n";
        sc(SYS_WRITE,debug_fd,(long)header,sizeof header-1,0,0,0);
    }
}
static unsigned debug_number(char *out,unsigned value) {
    char reverse[12];unsigned n=0,i=0;
    do {reverse[n++]=(char)('0'+value%10u);value/=10u;}while(value && n<sizeof reverse);
    while(n)out[i++]=reverse[--n];
    return i;
}
static void debug_event(unsigned ms,const char *raw,unsigned raw_len,
                        int accepted,unsigned status,int y256) {
    if(debug_fd<0 || debug_count++>=1000)return;
    char line[150];unsigned n=0;
    n+=debug_number(line+n,ms);
    line[n++]=' ';
    if(raw_len>64)raw_len=64;
    for(unsigned i=0;i<raw_len;++i) {
        unsigned char c=(unsigned char)raw[i];
        line[n++]=(c>=32 && c<=126)?(char)c:'?';
    }
    line[n++]=' ';
    line[n++]=(char)('0'+!!accepted);
    line[n++]=' ';
    n+=debug_number(line+n,status);
    line[n++]=' ';
    if(y256<0) {line[n++]='-'; y256=-y256;}
    n+=debug_number(line+n,(unsigned)y256);
    line[n++]='\\n';
    sc(SYS_WRITE,debug_fd,(long)line,n,0,0,0);
}
static int bind_input(void) {
    struct { unsigned short family;char path[108]; }addr={1,INPUT_SOCKET};
    int fd=(int)sc(SYS_SOCKET,1,1,0,0,0,0);
    if(fd<0)return -1;
    sc(SYS_UNLINK,(long)INPUT_SOCKET,0,0,0,0,0);
    if(!sc(SYS_BIND,fd,(long)&addr,sizeof addr,0,0,0))return fd;
    sc(SYS_CLOSE,fd,0,0,0,0,0);
    return -1;
}
int q2_main(void) {
    unsigned var[40],fix[17];
    int fb=(int)sc(SYS_OPEN,(long)"/dev/fb0",O_RDWR|O_CLOEXEC,0,0,0,0);
    if(fb<0)return 1;
    if(sc(SYS_IOCTL,fb,FB_VSCREEN,(long)var,0,0,0) ||
       sc(SYS_IOCTL,fb,FB_FSCREEN,(long)fix,0,0,0)) {
        sc(SYS_CLOSE,fb,0,0,0,0,0);return 1;
    }
    unsigned stride=fix[11]/sizeof(u32),front=var[5];
    if(var[0]!=SCREEN_H||var[1]!=SCREEN_W||var[4]||var[6]!=32||
       var[8]!=16||fix[11]%sizeof(u32)||stride<SCREEN_H) {
        sc(SYS_CLOSE,fb,0,0,0,0,0);return 1;
    }
    unsigned rows=fix[5]/fix[11];
    if(rows>var[3])rows=var[3];
    if(rows<SCREEN_W||front>rows-SCREEN_W) {
        sc(SYS_CLOSE,fb,0,0,0,0,0);return 1;
    }
    long mapped=sc(SYS_MMAP,0,(long)fix[5],3,1,fb,0);
    if(mapped<0 && mapped>=-4095) {
        sc(SYS_CLOSE,fb,0,0,0,0,0);return 1;
    }
    u32 *pixels=(u32*)mapped;
    int input=bind_input();
    if(input<0) {
        sc(SYS_MUNMAP,mapped,(long)fix[5],0,0,0,0);
        sc(SYS_CLOSE,fb,0,0,0,0,0);return 1;
    }
    load();
    app_init(&app,now_ms(),saved.best);
    debug_begin();
    unsigned visible=front;
    int can_pan=rows>=2*SCREEN_W && (front==0||front==SCREEN_W);
    struct {int fd;short events,revents;} poll_input={input,1,0};
    unsigned previous=now_ms(),deadline=previous;
    int redraw=1;
    while(!app.quit) {
        unsigned now=now_ms();
        int animated=app_animated(&app);
        if((animated&&(int)(deadline-now)<=0)||redraw) {
            unsigned elapsed=now-previous;
            if(elapsed>2*FRAME_MS)elapsed=2*FRAME_MS;
            previous=now;
            deadline=now+FRAME_MS;
            if(animated)app_tick(&app,elapsed);
            unsigned target=can_pan?(visible?0:SCREEN_W):visible;
            app_render(&app,pixels+target*stride,stride);
            if(can_pan){
                var[5]=target;
                if(!sc(SYS_IOCTL,fb,FB_PAN,(long)var,0,0,0))
                    visible=target;
                else {
                    can_pan=0;
                    app_render(&app,pixels+visible*stride,stride);
                }
            }
            if(app.dirty)save();
            redraw=0;
        }
        animated=app_animated(&app);
        int remaining=(int)(deadline-now_ms());
        if(remaining<0)remaining=0;
        long ready=sc(SYS_POLL,(long)&poll_input,1,animated?remaining:-1,0,0,0);
        if(ready==0||ready==-4)continue;
        if(ready<0||!(poll_input.revents&1))break;
        char message[64];
        long n=sc(SYS_RECV,input,(long)message,sizeof message-1,MSG_DONTWAIT,0,0);
        if(n>0){
            message[n]=0;
            int accepted=app_input(&app,message);
            debug_event(now_ms(),message,(unsigned)n,accepted,
                        (unsigned)app.runner.status,app.y256);
            if(accepted)redraw=1;
        }
    }
    if(app.dirty)save();
    if(debug_fd>=0)sc(SYS_CLOSE,debug_fd,0,0,0,0,0);
    if(visible!=front){
        var[5]=front;
        sc(SYS_IOCTL,fb,FB_PAN,(long)var,0,0,0);
    }
    sc(SYS_CLOSE,input,0,0,0,0,0);
    sc(SYS_UNLINK,(long)INPUT_SOCKET,0,0,0,0,0);
    sc(SYS_MUNMAP,mapped,(long)fix[5],0,0,0,0);
    sc(SYS_CLOSE,fb,0,0,0,0,0);
    return app.quit?0:1;
}
