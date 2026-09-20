/*
 * dj_booth.c — DJ Booth with Pitch Control
 * DE1-SoC — Multi-track playback with volume, track switching, and pitch
 *
 * CONTROL MAPPING
 * ─────────────────────────────────────────────────────────
 * SW[9:7]  Volume level  0-7 (mute → full)
 * SW[6:4]  Pitch factor  0 = 0.8x, 7 = 1.2x (fractional)
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

#include "audio_data.h"
#include "audio_data2.h"
#include "audio_data3.h"
#include "audio_data4.h"
#include "audio_data5.h"

#define AUDIO_BASE  0xFF203040
#define SW_BASE     0xFF200040
#define KEY_EDGECAP 0xFF20005C
#define LED_BASE    0xFF200000

struct audio_t {
    volatile unsigned int  control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char wsrc;
    volatile unsigned char wslc;
    volatile unsigned int  ldata;
    volatile unsigned int  rdata;
};

static struct audio_t *const audiop   = (struct audio_t *)AUDIO_BASE;
static volatile int   *const SW_ptr   = (volatile int *)SW_BASE;
static volatile int   *const EDGE_ptr = (volatile int *)KEY_EDGECAP;
static volatile int   *const LED_ptr  = (volatile int *)LED_BASE;

#define VOL_MASK       0x380  /* SW[9:7] volume */
#define VOL_SHIFT      7
#define PITCH_MASK     0x70   /* SW[6:4] pitch */
#define PITCH_SHIFT    4
#define TRACK_SEL_MASK 0x007  /* SW[2:0] track */

#define NUM_TRACKS 5

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

/* ─── Helpers ──────────────────────────────── */
static inline int get_volume(void) {
    return ((*SW_ptr) & VOL_MASK) >> VOL_SHIFT;
}

static inline int get_pitch_sw(void) {
    return ((*SW_ptr) & PITCH_MASK) >> PITCH_SHIFT;
}

static inline int get_track_sw(void) {
    int t = (*SW_ptr) & TRACK_SEL_MASK;
    return (t >= NUM_TRACKS) ? NUM_TRACKS - 1 : t;
}

static inline int apply_volume(int sample, int level) {
    if (level == 0) return 0;
    if (level == 7) return sample;
    return (int)(((long long)sample * level) / 7);
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

/* Map SW[6:4] (0-7) to pitch factor 0.8x → 1.2x */
static float pitch_from_sw(int sw_val) {
    return 0.8f + 0.057142857f * sw_val; // 0.8 + (sw_val * 0.4/7)
}

/* ═══════════════════════════════════════════════════════════ */
int main(void) {
    audio_init();

    int current_track  = 0;
    int last_sw_track  = 0;
    float playback_index = 0.0f; // use float for pitch interpolation
    int paused         = 0;
    int vol, sw_pitch;
    float pitch_factor;
    int sample, sample_l, sample_r;

    while (1) {
        int keys = check_keys();

        if (keys & 0x1) { /* KEY0 restart */
            playback_index = 0.0f;
            paused = 0;
        }
        if (keys & 0x2) { /* KEY1 pause/resume */
            paused = !paused;
        }
        if (keys & 0x4) { /* KEY2 previous track */
            current_track = (current_track - 1 + NUM_TRACKS) % NUM_TRACKS;
            playback_index = 0.0f;
        }
        if (keys & 0x8) { /* KEY3 next track */
            current_track = (current_track + 1) % NUM_TRACKS;
            playback_index = 0.0f;
        }

        int sw_track = get_track_sw();
        if (sw_track != last_sw_track) {
            last_sw_track = sw_track;
            current_track = sw_track;
            playback_index = 0.0f;
        }

        vol = get_volume();
        update_leds(vol, paused);

        if (paused) continue;

        while (audiop->wsrc == 0 || audiop->wslc == 0);

        /* Pitch control */
        sw_pitch = get_pitch_sw();
        pitch_factor = pitch_from_sw(sw_pitch);

        /* Linear interpolation for fractional index */
        int idx = (int)playback_index;
        int next_idx = (idx + 1) % track_len[current_track];
        float frac = playback_index - idx;
        sample = track_data[current_track][idx] +
                 (int)((track_data[current_track][next_idx] - track_data[current_track][idx]) * frac);

        sample_l = apply_volume(sample, vol);
        sample_r = apply_volume(sample, vol);

        audiop->ldata = (unsigned int)sample_l;
        audiop->rdata = (unsigned int)sample_r;

        playback_index += pitch_factor;
        if (playback_index >= track_len[current_track])
            playback_index -= track_len[current_track]; // wrap-around
    }

    return 0;
}