/*
 * dj_booth.c
 * DE1-SoC DJ Booth — Multi-track playback
 * Nios V Processor
 *
 * CONTROL MAPPING
 * ─────────────────────────────────────────────────────────
 * SW[9:7]  Volume level  0-7 (mute → full)
 * SW[2:0]  Track select  0-4
 *
 * KEY0     Restart current track from beginning
 * KEY1     Pause / Resume toggle
 * KEY2     Previous track
 * KEY3     Next track
 *
 * LED[6:0] Volume bar
 * LED[9]   Paused indicator
 */

/*
 * dj_booth.c — FIXED
 */

#include "audio_data.h"
#include "audio_data2.h"
#include "audio_data3.h"
#include "audio_data4.h"
#include "audio_data5.h"

/* ─── Peripheral base addresses ────────────────────────── */
#define AUDIO_BASE  0xFF203040
#define SW_BASE     0xFF200040
#define KEY_EDGECAP 0xFF20005C
#define LED_BASE    0xFF200000

/* ─── Audio peripheral registers ───────────────────────── */
struct audio_t {
    volatile unsigned int  control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char wsrc;
    volatile unsigned char wslc;
    volatile unsigned int  ldata;
    volatile unsigned int  rdata;
};

/* ─── Peripheral pointers ───────────────────────────────── */
static struct audio_t *const audiop   = (struct audio_t *)AUDIO_BASE;
static volatile int   *const SW_ptr   = (volatile int *)SW_BASE;
static volatile int   *const EDGE_ptr = (volatile int *)KEY_EDGECAP;
static volatile int   *const LED_ptr  = (volatile int *)LED_BASE;

/* ─── SW masks ──────────────────────────────────────────── */
#define VOL_MASK       0x380
#define VOL_SHIFT      7
#define TRACK_SEL_MASK 0x007

#define NUM_TRACKS 5

/* ─── Track table ───────────────────────────────────────── */
static const int * const track_data[NUM_TRACKS] = {
    audio_samples,
    audio_samples2,
    audio_samples3,
    audio_samples4,
    audio_samples5,
};

static const int track_len[NUM_TRACKS] = {
    AUDIO_LEN,
    AUDIO_LEN2,
    AUDIO_LEN3,
    AUDIO_LEN4,
    AUDIO_LEN5,
};

/* ─── Helpers ───────────────────────────────────────────── */
static inline int get_volume(void) {
    return ((*SW_ptr) & VOL_MASK) >> VOL_SHIFT;
}

static inline int get_track_sw(void) {
    int t = (*SW_ptr) & TRACK_SEL_MASK;
    return (t >= NUM_TRACKS) ? NUM_TRACKS - 1 : t;
}

/* FIXED volume scaling */
static inline int apply_volume(int sample, int level) {
    if (level == 0) return 0;
    return (int)(((long long)sample * level) / 7);
}

/*Prevent clipping distortion */
static inline int clamp24(int x) {
    if (x >  0x7FFFFF) return  0x7FFFFF;
    if (x < -0x800000) return -0x800000;
    return x;
}

static void update_leds(int vol, int paused) {
    int leds = (1 << vol) - 1;
    if (paused) leds |= (1 << 9);
    *LED_ptr = leds;
}

static int check_keys(void) {
    int edge = *EDGE_ptr;
    if (edge) *EDGE_ptr = edge;
    return edge & 0xF;
}

static void audio_init(void) {
    audiop->control = 0x8;
    audiop->control = 0x0;
}

/* ═══════════════════════════════════════════════════════════ */
int main(void) {
    audio_init();

    int current_track  = 0;
    int last_sw_track  = 0;
    int playback_index = 0;
    int paused         = 0;

    int sample, sample_l, sample_r;

    while (1) {

        /* ── Keys ── */
        int keys = check_keys();

        if (keys & 0x1) {   /* restart */
            playback_index = 0;
            paused = 0;
        }

        if (keys & 0x2) {   /* pause */
            paused = !paused;
        }

        if (keys & 0x4) {   /* prev track */
            current_track  = (current_track - 1 + NUM_TRACKS) % NUM_TRACKS;
            playback_index = 0;
        }

        if (keys & 0x8) {   /* next track */
            current_track  = (current_track + 1) % NUM_TRACKS;
            playback_index = 0;
        }

        /* ── SW track select ── */
        int sw_track = get_track_sw();
        if (sw_track != last_sw_track) {
            last_sw_track  = sw_track;
            current_track  = sw_track;
            playback_index = 0;
        }

        /* ── LEDs ── */
        int vol = get_volume();
        update_leds(vol, paused);

        if (paused) continue;

        /* ── Wait for FIFO space ── */
        while (audiop->wsrc == 0 || audiop->wslc == 0);

        /* ── Get sample ── */
        sample = track_data[current_track][playback_index];

        /* ── Apply volume + clamp ── */
        sample_l = clamp24(apply_volume(sample, vol));
        sample_r = sample_l;

        /* ── Output ── */
        audiop->ldata = (unsigned int)sample_l;
        audiop->rdata = (unsigned int)sample_r;

        /*  advance EVERY cycle (no upsampling) */
        playback_index = (playback_index + 1) % track_len[current_track];
    }

    return 0;
}
