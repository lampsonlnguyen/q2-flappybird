/* SPDX-License-Identifier: GPL-3.0-only
 * Short WAV sound effects for Flappy Bird.
 * ALSA device handoff, DAC mute and retry policy follow Diamond Bond's
 * Q2 Groovebox (GPL-3.0-only): https://github.com/DiamondBond/q2-groovebox
 */
#define _GNU_SOURCE
#include "audio.h"
#include <alsa/asoundlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

enum { RATE = 44100, PERIOD = 256, EFFECTS = 4, MAX_FRAMES = 96000 };
#define DAC "/dev/shanling_dac"
#define DAC_PCM 0xc0044d1bu
#define DAC_MUTE 0xc0044d1fu
static const char *const names[EFFECTS] = {
    "sfx_wing.wav", "sfx_point.wav", "sfx_die.wav", "sfx_swooshing.wav"
};
typedef struct { int16_t *pcm; unsigned length, rate, step; } Effect;
static Effect samples[EFFECTS];
static snd_pcm_t *pcm;
static pthread_t worker;
static int jack, gain = 65536;
static int active, running;
static int pending;
static volatile int *mono, *swap;

static unsigned le16(const unsigned char *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static unsigned le32(const unsigned char *p) {
    return le16(p) | le16(p + 2) << 16;
}
/* Read 16-bit PCM RIFF/WAVE, including optional LIST or metadata chunks.
 * The supplied files contain mono/stereo, 44.1/48 kHz and a LIST chunk.
 * Convert stereo to mono once; the 44.1 kHz audio worker resamples on play.
 */
static void read_wav(const char *dir, int id) {
    char path[512];
    snprintf(path, sizeof path, "%s/sounds/%s", dir, names[id]);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    unsigned char riff[12];
    unsigned format = 0, channels = 0, sample_rate = 0, bits = 0;
    if (fread(riff, 1, sizeof riff, f) != sizeof riff ||
        memcmp(riff, "RIFF", 4) || memcmp(riff + 8, "WAVE", 4)) {
        fclose(f);
        return;
    }
    for (int chunk = 0; chunk < 64; ++chunk) {
        unsigned char h[8];
        if (fread(h, 1, sizeof h, f) != sizeof h) break;
        unsigned bytes = le32(h + 4);
        if (bytes > 2000000) break; /* Reject unreasonable or corrupt WAVs. */
        if (!memcmp(h, "fmt ", 4) && bytes >= 16) {
            unsigned char fmt[16];
            if (fread(fmt, 1, sizeof fmt, f) != sizeof fmt) break;
            format = le16(fmt);
            channels = le16(fmt + 2);
            sample_rate = le32(fmt + 4);
            bits = le16(fmt + 14);
            if (bytes > 16 && fseek(f, (long)(bytes - 16), SEEK_CUR)) break;
        } else if (!memcmp(h, "data", 4)) {
            if (format != 1 || (channels != 1 && channels != 2) ||
                bits != 16 || sample_rate < 8000 || sample_rate > 96000) break;
            unsigned frames = bytes / (2u * channels);
            if (frames > MAX_FRAMES) frames = MAX_FRAMES;
            if (!frames) break;
            size_t raw_bytes = (size_t)frames * channels * 2;
            unsigned char *raw = malloc(raw_bytes);
            int16_t *mono_pcm = malloc((size_t)frames * sizeof(int16_t));
            if (raw && mono_pcm && fread(raw, 1, raw_bytes, f) == raw_bytes) {
                for (unsigned i = 0; i < frames; ++i) {
                    const unsigned char *p = raw + (size_t)i * channels * 2;
                    int left = (int16_t)le16(p);
                    int v = left;
                    if (channels == 2)
                        v = (left + (int)(int16_t)le16(p + 2)) / 2;
                    mono_pcm[i] = (int16_t)v;
                }
                samples[id].pcm = mono_pcm;
                samples[id].length = frames;
                samples[id].rate = sample_rate;
                samples[id].step = (unsigned)(((uint64_t)sample_rate << 16) / RATE);
                mono_pcm = 0;
            }
            free(raw);
            free(mono_pcm);
            break;
        } else if (fseek(f, (long)bytes, SEEK_CUR)) break;
        if ((bytes & 1u) && fseek(f, 1, SEEK_CUR)) break;
    }
    fclose(f);
}
static void dac_set(unsigned long request, int v) {
    int fd = open(DAC, O_RDWR | O_CLOEXEC);
    if (fd >= 0) { ioctl(fd, request, &v); close(fd); }
}
static const char *usb_pcm(char *name) {
    char data[1024];
    int fd = open("/proc/asound/cards", O_RDONLY | O_CLOEXEC);
    int n = fd >= 0 ? (int)read(fd, data, sizeof data - 1) : -1;
    if (fd >= 0) close(fd);
    data[n > 0 ? n : 0] = 0;
    char *p = strstr(data, ": USB-Audio - ");
    if (!p) return "plughw:1,0";
    while (p > data && p[-1] != '\n') --p;
    snprintf(name, 16, "plughw:%d,0", atoi(p));
    return name;
}
static volatile int *map_flag(const char *file) {
    int fd = open(file, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    void *p = mmap(0, sizeof(int), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    return p == MAP_FAILED ? 0 : p;
}
/* Q2's soft Bluetooth/USB volume curve, the same as Groovebox. */
static int soft_gain(const char *volume) {
    if (!volume || !*volume || *volume == 'h') return 65536;
    int v = atoi(volume);
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return 65536 * (v < 50 ? 2 * v : 18 * v - 800) / 1000;
}
static void *audio_worker(void *unused) {
    (void)unused;
    short out[PERIOD * 2];
    uint64_t phase = 0;
    int current = -1;
    unsigned passes = 0;
    while (__atomic_load_n(&running, __ATOMIC_ACQUIRE)) {
        int next = __atomic_exchange_n(&pending, 0, __ATOMIC_ACQ_REL);
        if (next >= 1 && next <= EFFECTS) {
            current = next - 1;
            phase = 0;
        }
        if (!(passes++ % 100)) {
            if (!mono) mono = map_flag("/tmp/q2mono");
            if (!swap) swap = map_flag("/tmp/q2swap");
        }
        for (unsigned i = 0; i < PERIOD; ++i) {
            int v = 0;
            if (current >= 0 && samples[current].pcm) {
                const Effect *effect = &samples[current];
                unsigned index = (unsigned)(phase >> 16);
                if (index < effect->length) {
                    int a = effect->pcm[index];
                    int b = effect->pcm[index + 1 < effect->length ? index + 1 : index];
                    unsigned frac = (unsigned)phase & 65535u;
                    v = a + (int)(((int64_t)(b - a) * frac) >> 16);
                    phase += effect->step;
                } else current = -1;
            } else current = -1;
            v = (int)((int64_t)v * gain / 65536);
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            out[2 * i] = out[2 * i + 1] = (short)v;
            if (mono && *mono > 0)
                out[2 * i] = out[2 * i + 1] =
                    (short)((out[2 * i] + out[2 * i + 1]) / 2);
            if (swap && *swap > 0) {
                short tmp = out[2 * i];
                out[2 * i] = out[2 * i + 1];
                out[2 * i + 1] = tmp;
            }
        }
        snd_pcm_sframes_t n = snd_pcm_writei(pcm, out, PERIOD);
        if (n < 0 && snd_pcm_recover(pcm, (int)n, 1) < 0) break;
    }
    return 0;
}
int audio_start(const char *app_path, const char *device, const char *volume) {
    char dir[384];
    snprintf(dir, sizeof dir, "%s", app_path ? app_path : "");
    char *slash = strrchr(dir, '/');
    if (slash) *slash = 0;
    else snprintf(dir, sizeof dir, ".");
    for (int i = 0; i < EFFECTS; ++i) {
        read_wav(dir, i);
        /* Older launchers may supply only the executable basename. */
        if (!samples[i].pcm) read_wav("/mnt/mmc/.flappybird", i);
    }
    char usb[16];
    const char *dev = device && *device ? device : "plughw:0,0";
    if (!strcmp(dev, "-")) return 0;
    if (!strcmp(dev, "usb")) dev = usb_pcm(usb);
    jack = !strcmp(dev, "plughw:0,0");
    gain = soft_gain(volume);
    setenv("LIBASOUND_THREAD_SAFE", "0", 1);
    int err = -1;
    for (int i = 0; i < (jack ? 20 : 50); ++i) {
        err = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0);
        if (!err) break;
        pcm = 0;
        usleep(100000);
    }
    if (err) return 0;
    for (int i = 0; ; ++i) {
        err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE,
                                 SND_PCM_ACCESS_RW_INTERLEAVED, 2, RATE, 1,
                                 4 * PERIOD * 1000000 / RATE);
        if (err != -EBUSY || jack || i >= 49) break;
        usleep(100000);
    }
    if (err) { snd_pcm_close(pcm); pcm = 0; return 0; }
    if (jack) { dac_set(DAC_PCM, 0); dac_set(DAC_MUTE, 0); }
    __atomic_store_n(&running, 1, __ATOMIC_RELEASE);
    if (pthread_create(&worker, 0, audio_worker, 0)) {
        __atomic_store_n(&running, 0, __ATOMIC_RELEASE);
        snd_pcm_close(pcm); pcm = 0;
        if (jack) dac_set(DAC_MUTE, 1);
        return 0;
    }
    active = 1;
    return 1;
}
void audio_play(int event) {
    if (active && event >= 1 && event <= EFFECTS)
        __atomic_store_n(&pending, event, __ATOMIC_RELEASE);
}
void audio_stop(void) {
    if (pcm) {
        if (active) {
            __atomic_store_n(&running, 0, __ATOMIC_RELEASE);
            snd_pcm_drop(pcm); /* Unblock a write if needed. */
            pthread_join(worker, 0);
            active = 0;
        }
        snd_pcm_close(pcm);
        pcm = 0;
    }
    if (mono) { munmap((void *)mono, sizeof(int)); mono = 0; }
    if (swap) { munmap((void *)swap, sizeof(int)); swap = 0; }
    if (jack) dac_set(DAC_MUTE, 1);
    for (int i = 0; i < EFFECTS; ++i) {
        free(samples[i].pcm);
        samples[i].pcm = 0;
        samples[i].length = samples[i].rate = samples[i].step = 0;
    }
}
