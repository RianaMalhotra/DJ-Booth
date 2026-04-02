/*
 * dj_booth_interrupt.c
 * DJ Booth with proper Nios V interrupts matching your assembly pattern.
 *
 * INTERRUPT MODEL (from your assembly reference):
 *   IRQ 16  = Timer    (bit 16 of mie)
 *   IRQ 18  = Audio    (bit 18 of mie)
 *   mcause low 31 bits == IRQ line number
 *   mtvec   = address of interrupt_handler
 *   mstatus bit 3 = global interrupt enable
 *
 * CONTROL MAPPING
 * SW[9:7]  Volume level  0-7
 * SW[6:4]  Pitch factor  0=0.8x ... 7=1.2x
 * SW[0]    Reverse
 * SW[1]    Echo
 * SW[2]    Crossfade
 * SW[3]    Back to main screen
 */

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "audio_data.h"
#include "audio_data2.h"
#include "audio_data3.h"
#include "audio_data4.h"
#include "audio_data5.h"

/* ─── Peripheral addresses ──────────────────────────────── */
#define AUDIO_BASE  0xFF203040
#define SW_BASE     0xFF200040
#define KEY_EDGECAP 0xFF20005C
#define KEY_BASE    0xFF200050
#define LED_BASE    0xFF200000
#define FB_ADDR     0xFF203020
#define PS2_BASE    0xFF200100
#define TIMER_BASE  0xFF202000

/* ─── Screen ────────────────────────────────────────────── */
#define SCREEN_W 320
#define SCREEN_H 240

/* ─── Colours ───────────────────────────────────────────── */
#define BG_COLOUR_S 0xFC0E
#define BG_COLOUR_M 0xECBB
#define P_PURPLE    0x8C9F
#define P_BLUE      0x7D9F
#define B_BLUE      0x0E5F
#define ORANGE      0xFBE0
#define TEAL        0x0410
#define B_TEAL      0x4ED5
#define GREEN       0xA6D4
#define RED         0xE2C8
#define L_GREY      0xAD55
#define YELLOW      0xFEE0
#define GREY        0x8410
#define BLACK       0x0000
#define WHITE       0xFFFF
#define L_BLACK     0x528A
#define BLUE        0x6F7F
#define PINK        0xE5B8
#define PURP        0xBD5B

/* ─── Font ──────────────────────────────────────────────── */
#define FONT_W 5
#define FONT_H 7
#define DEG2RAD(deg) ((deg) * M_PI / 180.0)

const unsigned char FONT[128][7] = {
    [' '] = {0,0,0,0,0,0,0},
    ['W'] = {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001},
    ['e'] = {0b00000,0b00000,0b01110,0b10001,0b11111,0b10000,0b01111},
    ['l'] = {0b00110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110},
    ['c'] = {0b00000,0b00000,0b01110,0b10000,0b10000,0b10000,0b01111},
    ['o'] = {0b00000,0b00000,0b01110,0b10001,0b10001,0b10001,0b01110},
    ['m'] = {0b00000,0b00000,0b11010,0b10101,0b10101,0b10101,0b10101},
    ['t'] = {0b00100,0b00100,0b11111,0b00100,0b00100,0b00100,0b00011},
    ['C'] = {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110},
    ['h'] = {0b10000,0b10000,0b10110,0b11001,0b10001,0b10001,0b10001},
    ['&'] = {0b01100,0b10010,0b10100,0b01000,0b10101,0b10010,0b01101},
    ['R'] = {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001},
    ['i'] = {0b00100,0b00000,0b01100,0b00100,0b00100,0b00100,0b01110},
    ['a'] = {0b00000,0b00000,0b01110,0b00001,0b01111,0b10001,0b01111},
    ['n'] = {0b00000,0b00000,0b10110,0b11001,0b10001,0b10001,0b10001},
    ['\'']= {0b00100,0b00100,0b01000,0b00000,0b00000,0b00000,0b00000},
    ['s'] = {0b00000,0b00000,0b01111,0b10000,0b01110,0b00001,0b11110},
    ['D'] = {0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110},
    ['J'] = {0b00001,0b00001,0b00001,0b00001,0b10001,0b10001,0b01110},
    ['B'] = {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110},
    ['E'] = {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111},
    ['N'] = {0b10001,0b11001,0b10101,0b10101,0b10011,0b10001,0b10001},
    ['T'] = {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100},
    ['P'] = {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000},
    ['F'] = {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000},
    ['V'] = {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100},
    ['S'] = {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110},
    ['H'] = {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},
    ['O'] = {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110},
    ['A'] = {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},
    ['L'] = {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111},
    ['r'] = {0b00000,0b00000,0b10110,0b11001,0b10000,0b10000,0b10000},
    ['k'] = {0b10000,0b10000,0b10010,0b10100,0b11000,0b10100,0b10010},
    ['1'] = {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110},
    ['2'] = {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111},
    ['3'] = {0b11110,0b00001,0b00001,0b01110,0b00001,0b00001,0b11110},
    ['4'] = {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010},
    ['5'] = {0b11111,0b10000,0b10000,0b11110,0b00001,0b00001,0b11110},
};

/* ─── Timer registers ───────────────────────────────────── */
#define TIMER_STATUS    (*(volatile int *)(TIMER_BASE + 0x00))
#define TIMER_CONTROL   (*(volatile int *)(TIMER_BASE + 0x04))
#define TIMER_START_LO  (*(volatile int *)(TIMER_BASE + 0x08))
#define TIMER_START_HI  (*(volatile int *)(TIMER_BASE + 0x0C))
#define TIMER_PERIOD    133000

/* ─── Audio peripheral ──────────────────────────────────── */
struct audio_t {
    volatile unsigned int  control;
    volatile unsigned char rarc;
    volatile unsigned char ralc;
    volatile unsigned char wsrc;
    volatile unsigned char wslc;
    volatile unsigned int  ldata;
    volatile unsigned int  rdata;
};
#define AUDIO_WIE (1u << 1)   /* Write Interrupt Enable */
#define AUDIO_CW  (1u << 3)   /* Clear write FIFOs      */

/* ─── Peripheral pointers ───────────────────────────────── */
static struct audio_t *const audiop   = (struct audio_t *)AUDIO_BASE;
static volatile int   *const SW_ptr   = (volatile int *)SW_BASE;
static volatile int   *const EDGE_ptr = (volatile int *)KEY_EDGECAP;
static volatile int   *const KEY_ptr  = (volatile int *)KEY_BASE;
static volatile int   *const LED_ptr  = (volatile int *)LED_BASE;
static volatile int   *const PS2_ptr  = (volatile int *)PS2_BASE;

/* ─── SW masks ──────────────────────────────────────────── */
#define VOL_MASK       0x380
#define VOL_SHIFT      7
#define PITCH_MASK     0x070
#define PITCH_SHIFT    4
#define CROSSFADE_MASK 0x004
#define ECHO_MASK      0x002
#define REVERSE_MASK   0x001

/* ─── Echo ──────────────────────────────────────────────── */
#define ECHO_BUF_SIZE 24000
#define ECHO_FEEDBACK 200
#define ECHO_WET      180
#define ECHO_DELAY    8000

static int echo_buf_l[ECHO_BUF_SIZE];
static int echo_buf_r[ECHO_BUF_SIZE];
static int echo_pos = 0;

#define CLAMP24(x) ((x) >  8388607 ?  8388607 : \
                   ((x) < -8388608 ? -8388608 : (x)))

/* ─── Crossfade ─────────────────────────────────────────── */
static volatile int cf_alpha   = 65536;
static volatile int cf_active  = 0;
static volatile int next_track = 0;

/* ─── Track table ───────────────────────────────────────── */
#define NUM_TRACKS 5
static const int * const track_data[NUM_TRACKS] = {
    audio_samples, audio_samples2, audio_samples3,
    audio_samples4, audio_samples5,
};
static const int track_len[NUM_TRACKS] = {
    AUDIO_LEN, AUDIO_LEN2, AUDIO_LEN3, AUDIO_LEN4, AUDIO_LEN5,
};
static volatile int current_track     = 0;
static volatile int playback_index_fp = 0;
static volatile int idx_b_fp          = 0;

/* ─── VGA buffers ───────────────────────────────────────── */
volatile short int *pixel_buffer_start;
short int Buffer1[240][512];
short int Buffer2[240][512];

/* ─── Precomputed audio params (set in main, read in ISR) ── */
static volatile int g_paused   = 1;
static volatile int g_pitch_fp = 256;
static volatile int g_vol      = 7;
static volatile int g_reverse  = 0;
static volatile int g_echo_on  = 0;

/* ─── VGA trigger ───────────────────────────────────────── */
static volatile int vga_pending = 0;

/* ─── UI state ──────────────────────────────────────────── */
static int   LOADING   = 1;
static int   MAIN      = 0;
static int   IND       = 0;
static int   NUM_COL   = 3;
static int   TRACK_NUM = 1;
static int   DRAW      = 2;
static int   PAUSE     = 0;
static float theta     = 0;
static float theta1    = 0;
static short int GLOBAL_C;
static short int COLOURS[3] = {B_BLUE, ORANGE, B_TEAL};

/* ════════════════════════════════════════════════════════════
 * FORWARD DECLARATIONS
 * ════════════════════════════════════════════════════════════ */
void plot_pixel(int x, int y, short int colour);
void plot_scaled_pixel(int x, int y, short int colour, float scale);
void fill_background(short int colour);
void wait_for_vsync(void);
void draw_char(int x, int y, char c, short int fill, short int outline, float scale);
void draw_letters(int x, int y, const char *s, short int fill, short int outline, float scale);
void draw_letters_centered(int y, const char *s, short int fill, short int outline, float scale);
void draw_circle(int x, int y, int r, short int colour);
void draw_disc(int x, int y, int r, bool show_button, float th, volatile int *mouse_ptr, bool echo, bool blend, bool multi);
void draw_arc_highlight(int x, int y, int r, float c_deg, float s_deg, int m_thick, short int colour);
void draw_enter_button(int x, int y, int w, int h, bool changed);
void draw_exit_button(int x0, int y0, int w, int h, short int lc, short int oc, short int ic, int sc);
void draw_rectangle(int x, int y, int w, int h, short int colour);
void draw_letters_box(int x, int y, int w, int h, const char *s, short int fill, short int outline, float scale);
void draw_line(int x0, int y0, int x1, int y1, short int colour);
void draw_push_button(int x, int y, int w, int h, int r, short int oc, short int ic, int sc, int mode, short int on);
void draw_switch(int x, int y, int w, int h, short int base, short int off, short int on, int sx, int sy, int mode);
void draw_play_button(int x, int y_ctr, int h, int w, short int colour);
void draw_led(int x, int y, int w, int h, short int outer, short int inner, int sc, int mode, short int on);
void swap(int *a, int *b);
void check_sc_switch(volatile int *SW_ptr);
void vga_draw_frame(void);
void audio_isr(void);
void timer_isr(void);

/* ════════════════════════════════════════════════════════════
 * AUDIO HELPERS
 * ════════════════════════════════════════════════════════════ */
static inline int get_volume(void)   { return ((*SW_ptr) & VOL_MASK)   >> VOL_SHIFT;  }
static inline int get_pitch_sw(void) { return ((*SW_ptr) & PITCH_MASK) >> PITCH_SHIFT;}
static inline int get_reverse(void)  { return  (*SW_ptr) & REVERSE_MASK;              }
static inline int get_echo(void)     { return  (*SW_ptr) & ECHO_MASK;                 }
static inline int get_cf_sw(void)    { return  (*SW_ptr) & CROSSFADE_MASK;            }

static inline int apply_volume(int sample, int level) {
    if (level == 0) return 0;
    if (level == 7) return sample;
    return (int)(((long long)sample * (128 + level * 18)) >> 8);
}

static void update_leds(int vol, int paused, int reverse, int echo, int crossfading) {
    int leds = (1 << vol) - 1;
    if (paused)      leds |= (1 << 9);
    if (reverse)     leds |= (1 << 8);
    if (echo)        leds |= (1 << 7);
    if (crossfading) leds |= (1 << 6);
    *LED_ptr = leds;
}

static int check_keys(void) {
    int edge = *(EDGE_ptr);
    if (edge) *EDGE_ptr = edge;
    return edge & 0xF;
}

static void echo_clear(void) { echo_pos = 0; }

static void process_echo(int dry_l, int dry_r,
                         int *out_l, int *out_r, int echo_on) {
    int read_pos = echo_pos - ECHO_DELAY;
    if (read_pos < 0) read_pos += ECHO_BUF_SIZE;
    int delayed_l = echo_buf_l[read_pos];
    int delayed_r = echo_buf_r[read_pos];
    if (echo_on) {
        int fb_l = (int)(((long long)delayed_l * ECHO_FEEDBACK) >> 8);
        int fb_r = (int)(((long long)delayed_r * ECHO_FEEDBACK) >> 8);
        echo_buf_l[echo_pos] = CLAMP24(dry_l + fb_l);
        echo_buf_r[echo_pos] = CLAMP24(dry_r + fb_r);
        int dry_w = 256 - ECHO_WET;
        *out_l = (int)((((long long)dry_l * dry_w) + ((long long)delayed_l * ECHO_WET)) >> 8);
        *out_r = (int)((((long long)dry_r * dry_w) + ((long long)delayed_r * ECHO_WET)) >> 8);
    } else {
        echo_buf_l[echo_pos] = dry_l;
        echo_buf_r[echo_pos] = dry_r;
        *out_l = dry_l;
        *out_r = dry_r;
    }
    echo_pos++;
    if (echo_pos >= ECHO_BUF_SIZE) echo_pos = 0;
}

static float pitch_from_sw(int sw_val) {
    return 0.8f + 0.05714f * (float)sw_val;
}

static void switch_track(int target, int smooth) {
    if (target == current_track && !cf_active) return;
    if (smooth && !cf_active) {
        next_track = target;
        idx_b_fp   = 0;
        cf_alpha   = 65536;
        cf_active  = 1;
    } else if (!smooth) {
        current_track     = target;
        playback_index_fp = 0;
        cf_active         = 0;
        cf_alpha          = 65536;
        echo_clear();
    }
}

/* ════════════════════════════════════════════════════════════
 * AUDIO ISR
 * Called when IRQ 18 fires (audio FIFO has space).
 * Reads precomputed globals — NO float math here.
 * ════════════════════════════════════════════════════════════ */
void audio_isr(void) {
    if (g_paused) {
        // Fill FIFO with silence to prevent interrupt storm
        while (audiop->wsrc > 0 && audiop->wslc > 0) {
            audiop->ldata = 0;
            audiop->rdata = 0;
        }
        return;
    }

    int reverse  = g_reverse;
    int echo     = g_echo_on;
    int vol      = g_vol;
    int pitch_fp = g_pitch_fp;
    int tlen_a   = track_len[current_track] << 8;

    while (audiop->wsrc > 0 && audiop->wslc > 0) {
        int sample_l, sample_r;

        if (cf_active) {
            int tlen_b    = track_len[next_track] << 8;
            int sample_a  = track_data[current_track][playback_index_fp >> 8];
            int sample_b  = track_data[next_track][idx_b_fp >> 8];
            int blended   = (int)((((long long)sample_a * cf_alpha) >> 16)
                                + (((long long)sample_b * (65536 - cf_alpha)) >> 16));
            sample_l = apply_volume(blended, vol);
            sample_r = apply_volume(blended, vol);
            if (reverse) {
                playback_index_fp -= pitch_fp;
                if (playback_index_fp < 0) playback_index_fp += tlen_a;
                idx_b_fp -= pitch_fp;
                if (idx_b_fp < 0) idx_b_fp += tlen_b;
            } else {
                playback_index_fp += pitch_fp;
                if (playback_index_fp >= tlen_a) playback_index_fp -= tlen_a;
                idx_b_fp += pitch_fp;
                if (idx_b_fp >= tlen_b) idx_b_fp -= tlen_b;
            }
            cf_alpha -= 3;
            if (cf_alpha <= 0) {
                cf_alpha          = 65536;
                cf_active         = 0;
                current_track     = next_track;
                playback_index_fp = idx_b_fp;
                tlen_a            = tlen_b;
                echo_clear();
            }
        } else {
            int sample_a = track_data[current_track][playback_index_fp >> 8];
            sample_l = apply_volume(sample_a, vol);
            sample_r = apply_volume(sample_a, vol);
            if (reverse) {
                playback_index_fp -= pitch_fp;
                if (playback_index_fp < 0) playback_index_fp += tlen_a;
            } else {
                playback_index_fp += pitch_fp;
                if (playback_index_fp >= tlen_a) playback_index_fp -= tlen_a;
            }
        }

        int out_l, out_r;
        process_echo(sample_l, sample_r, &out_l, &out_r, echo);
        audiop->ldata = out_l;
        audiop->rdata = out_r;
    }
}

/* ════════════════════════════════════════════════════════════
 * TIMER ISR
 * Called when IRQ 16 fires. Clears TO bit, sets vga_pending.
 * ════════════════════════════════════════════════════════════ */
void timer_isr(void) {
    TIMER_STATUS = 0;   /* clear TO bit — same as your assembly sw x0, 0(TIMER) */
    vga_pending  = 1;
}

/* ════════════════════════════════════════════════════════════
 * INTERRUPT HANDLER
 *
 * ════════════════════════════════════════════════════════════ */
void __attribute__((interrupt("machine"))) interrupt_handler(void) {
    int mcause_val;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_val));

    /* Mask off bit 31 (interrupt vs exception flag), keep IRQ number */
    int cause = mcause_val & 0x7FFFFFFF;

    if (cause == 16) {
        timer_isr();
    } else if (cause == 21) {
        audio_isr();
    }
    /* __attribute__((interrupt("machine"))) emits mret automatically */
}

/* ════════════════════════════════════════════════════════════
 * INIT FUNCTIONS
 * ════════════════════════════════════════════════════════════ */
static void audio_init(void) {
    audiop->control = AUDIO_CW;
    audiop->control = AUDIO_CW;
    audiop->control = 0;
    /* Enable Write Interrupt — fires IRQ 18 when FIFO has space */
    audiop->control = AUDIO_WIE;
}

static void timer_init(void) {
    TIMER_STATUS   = 0;
    TIMER_START_LO = TIMER_PERIOD & 0xFFFF;
    TIMER_START_HI = (TIMER_PERIOD >> 16) & 0xFFFF;
    TIMER_CONTROL  = 0x7;  /* ITO=1 CONT=1 START=1, same as your assembly 0b0111 */
}

static void interrupts_init(void) {
    __asm__ volatile ("csrw mstatus, zero");

    __asm__ volatile (
        "la  t0, interrupt_handler  \n"
        "csrw mtvec, t0             \n"
    );

    /* bit 16 = timer IRQ, bit 18 = audio IRQ */
    __asm__ volatile (
    "li  t0, (1 << 16) | (1 << 21) \n"   /* timer + audio */
    "csrs mie, t0                  \n"
);

    /* Turn on global machine interrupt enable (bit 3 of mstatus) */
    __asm__ volatile (
        "li  t0, 0b1000             \n"
        "csrs mstatus, t0           \n"
    );
}

/* ════════════════════════════════════════════════════════════
 * main
 * ════════════════════════════════════════════════════════════ */
int main(void) {
    volatile int *pixel_ctrl_ptr = (int *)FB_ADDR;

    *(pixel_ctrl_ptr + 1) = (int)&Buffer1;
    wait_for_vsync();
    pixel_buffer_start = (volatile short int *)*pixel_ctrl_ptr;
    fill_background(BLACK);

    *(pixel_ctrl_ptr + 1) = (int)&Buffer2;
    pixel_buffer_start = (volatile short int *)*(pixel_ctrl_ptr + 1);
    fill_background(BLACK);

    echo_clear();
    audio_init();   /* enables audio WIE before interrupts go live */
    timer_init();
    interrupts_init();  /* IRQs now live — audio fills via IRQ 18 automatically */

    while (1) {
        /*
         * Precompute float/switch params here in main.
         * ISR reads these volatile globals — no float in the ISR hot path.
         */
        g_pitch_fp = (int)(pitch_from_sw(get_pitch_sw()) * 256.0f + 0.5f);
        g_vol      = get_volume();
        g_reverse  = get_reverse();
        g_echo_on  = get_echo();

        /* Key handling */
        int keys = check_keys();
        if (keys & 0x1) { playback_index_fp = 0; echo_clear(); }
        if (keys & 0x2) g_paused = !g_paused;
        if (keys & 0x4) {switch_track((current_track - 1 + NUM_TRACKS) % NUM_TRACKS, get_cf_sw());
                        TRACK_NUM = (TRACK_NUM == 1) ? 5 : TRACK_NUM - 1;
                        }
        if (keys & 0x8) {switch_track((current_track + 1) % NUM_TRACKS, get_cf_sw());
                        TRACK_NUM = (TRACK_NUM == 5) ? 1 : TRACK_NUM + 1;
                        }

        update_leds(g_vol, g_paused, g_reverse, g_echo_on, cf_active);

        /*
         * VGA redraws only when timer ISR sets vga_pending.
         * Audio continues uninterrupted via IRQ 18 during all drawing.
         */
        if (vga_pending) {
            vga_pending = 0;
            vga_draw_frame();
            wait_for_vsync();
            pixel_buffer_start = (volatile short int *)*(pixel_ctrl_ptr + 1);
        }
    }

    return 0;
}

/* ════════════════════════════════════════════════════════════
 * VGA DRAW FRAME
 * All process_audio() calls removed — handled by IRQ 18.
 * ════════════════════════════════════════════════════════════ */
void vga_draw_frame(void) {
    int keys = check_keys();
    int sw_data, key_data, led_data;
    int sw_sum, key_sum, led_sum;

    /* ── Splash screen ── */
    if (LOADING) {
        if (DRAW > 0) {
            fill_background(BG_COLOUR_S);
            draw_letters_centered(25, "Welcome to",      YELLOW, BLACK, 2);
            draw_letters_centered(45, "Chloe & Riana's", YELLOW, BLACK, 2);
            draw_letters_centered(65, "DJ Booth",        YELLOW, BLACK, 2);
            DRAW--;
        }
        theta  -= 15; theta1 -= 15;
        if (theta  >= 360) theta  = -360;
        if (theta1 >= 360) theta1 = -360;
        draw_disc(160, 155, 65, 1, theta, PS2_ptr, 0, 0, 0);
        if (*SW_ptr & 0x8) {
            draw_enter_button(160, 155, 65*1.2, 65*0.38, 1);
            while (*SW_ptr & 0x8) {}
            LOADING = 0; MAIN = 1; DRAW = 2;
            g_paused = 0;
        }
        return;
    }

    if (!MAIN) return;

    /* ── Main screen ── */
    int sw = *SW_ptr & 0xF;

    if (sw == 0b0001) {
        int keys = check_keys();

if (keys & 0x2) {
    PAUSE = 1;
}
        else if (!g_paused) { theta += 15; theta1 += 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(B_BLUE); draw_letters_centered(12, "REVERSE", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = B_BLUE;
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0, 0, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0001) DRAW = 2;

    } else if (sw == 0b0010) {
        int keys = check_keys();

if (keys & 0x2) {
    PAUSE = 1;
}
        else if (!g_paused) { theta -= 15; theta1 -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(TEAL); draw_letters_centered(12, "ECHO", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = TEAL;
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 1, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 1, 0, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0010) DRAW = 2;

    } else if (sw == 0b0100) {
        if (!g_paused) {theta -= 15; theta1 -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(ORANGE); draw_letters_centered(12, "CROSSFADE", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = ORANGE;
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0, 1, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0, 1, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0100) DRAW = 2;

    } else if (sw == 0b0011) {
        int keys = check_keys();

if (keys & 0x2) {
    PAUSE = 1;
}
        else if (!g_paused) { theta1 += 15; theta -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(P_PURPLE); draw_letters_centered(12, "REVERSE & ECHO", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = P_PURPLE;
        draw_disc(70, 105, 60, 0, theta1, PS2_ptr, 0, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 1, 0, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0011) DRAW = 2;

    } else if (sw == 0b0101) {
        int keys = check_keys();

if (keys & 0x2) {
    PAUSE = 1;
}
        if (!g_paused) { theta1 += 15; theta -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(B_TEAL); draw_letters_centered(12, "REVERSE & CROSSFADE", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = B_TEAL;
        draw_disc(70, 105, 60, 0, theta1, PS2_ptr, 0, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0, 1, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0101) DRAW = 2;

    } else if (sw == 0b0110) {
        if (!g_paused) {theta1 -= 15; theta -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(P_BLUE); draw_letters_centered(12, "ECHO & CROSSFADE", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = P_BLUE;
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 1, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0, 1, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0110) DRAW = 2;

    } else if (sw == 0b0111) {
        if (!g_paused) {theta1 += 25; theta -= 25;}
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        fill_background(COLOURS[IND % NUM_COL]);
        GLOBAL_C = COLOURS[IND % NUM_COL];
        IND++;
        draw_letters_centered(12, "ALL EFFECTS", YELLOW, BLACK, 2);
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0, 0, 1);
        draw_disc(250, 105, 60, 0, theta1, PS2_ptr, 0, 0, 1);
        draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0111) DRAW = 2;

    } else {
        if (!g_paused) { theta1 -= 15; theta -= 15; }
        if (theta >= 360) theta = -360;
        if (theta1 >= 360) theta1 = -360;
        if (DRAW > 0) { fill_background(BG_COLOUR_M); draw_letters_centered(12, "CRB", YELLOW, BLACK, 2); draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREY, 3); DRAW--; }
        GLOBAL_C = BG_COLOUR_M;
        draw_disc(70, 105, 60, 0, theta, PS2_ptr, 0, 0, 0);
        draw_disc(250, 105, 60, 0, theta, PS2_ptr, 0, 0, 0);
        check_sc_switch(SW_ptr);
        if ((*SW_ptr & 0xF) != 0b0000) DRAW = 2;
    }

    /* ── Push buttons ── */
    key_data = *KEY_ptr; key_sum = 0;
    for (int i = 285; i >= 186; i -= 33) {
        draw_push_button(i, 198, 30, 30, 9, BLACK, L_GREY, 2, (key_data >> key_sum) & 1, GREEN);
        key_sum++;
    }
    draw_char(297, 208, 'R', WHITE, BLACK, 1.5f);
    draw_char(265, 209, 'P', WHITE, BLACK, 1.5f);
    draw_char(231, 208, 'B', WHITE, BLACK, 1.5f);



/*if (keys & 0x8) {
    PAUSE = 0;
    TRACK_NUM = (TRACK_NUM == 5) ? 1 : TRACK_NUM + 1;
}

if (keys & 0x4) {
    PAUSE = 0;
    TRACK_NUM = (TRACK_NUM == 1) ? 5 : TRACK_NUM - 1;
}*/



    draw_rectangle(215, 175, 75, 20, GLOBAL_C);
    char track[20];
    sprintf(track, "Track %d", TRACK_NUM);
    draw_letters(222, 183, track, WHITE, BLACK, 1.5f);
    draw_char(199, 209, 'F', WHITE, BLACK, 1.5f);

    /* ── Switches ── */
    sw_data = *SW_ptr; sw_sum = 0;
    for (int i = 167; i >= 5; i -= 18) {
        draw_switch(i, 195, 13, 35, L_GREY, GREY, BLACK, 2, 3, (sw_data >> sw_sum) & 1);
        sw_sum++;
    }

    /* ── LEDs ── */
    led_data = *LED_ptr; led_sum = 0;
    for (int i = 171; i >= 9; i -= 18) {
        draw_led(i, 180, 6, 12, L_GREY, WHITE, 1, (led_data >> led_sum) & 1, RED);
        led_sum++;
    }

    /* ── Pause button ── */
    short int pause_col = g_paused ? RED : GREY;
    draw_rectangle(146-3, 42-3, 26+6, 26+6, WHITE);
    draw_rectangle(146, 42, 26, 26, pause_col);
    draw_line(153, 47, 153, 63, YELLOW); draw_line(154, 47, 154, 63, YELLOW); draw_line(155, 47, 155, 63, YELLOW);
    draw_line(163, 47, 163, 63, YELLOW); draw_line(164, 47, 164, 63, YELLOW); draw_line(165, 47, 165, 63, YELLOW);

    /* ── Play button ── */
    short int play_col = !g_paused ? GREEN: GREY;
    draw_rectangle(146-3, 75-3, 26+6, 26+6, WHITE);
    draw_rectangle(146, 75, 26, 26, play_col);
    draw_play_button(151, 89, 16, 18, YELLOW);

    /* ── Volume + ── */
    short int plus_col = g_vol == 0x7 ? PURP : GREY;
    draw_rectangle(146-3, 108-3, 26+6, 26+6, WHITE);
    draw_rectangle(146, 108, 26, 26, plus_col);
    draw_line(158, 113, 158, 129, YELLOW); draw_line(159, 113, 159, 129, YELLOW); draw_line(160, 113, 160, 129, YELLOW);
    draw_line(151, 120, 167, 120, YELLOW); draw_line(151, 121, 167, 121, YELLOW); draw_line(151, 122, 167, 122, YELLOW);

    /* ── Volume - ── */
    short int minus_col = g_vol == 0x0 ? PINK : GREY;
    draw_rectangle(146-3, 141-3, 26+6, 26+6, WHITE);
    draw_rectangle(146, 141, 26, 26, minus_col);
    draw_line(151, 153, 167, 153, YELLOW); draw_line(151, 154, 167, 154, YELLOW); draw_line(151, 155, 167, 155, YELLOW);
}

/* ════════════════════════════════════════════════════════════
 * DRAWING PRIMITIVES — process_audio() calls all removed
 * ════════════════════════════════════════════════════════════ */
void check_sc_switch(volatile int *SW_ptr) {
    if (*SW_ptr & 0x8) {
        draw_exit_button(290, 12, 16, 16, WHITE, WHITE, GREEN, 3);
        while (*SW_ptr & 0x8) {}
        LOADING = 1; MAIN = 0; DRAW = 2;
        g_paused = 1;
    }
}

void plot_pixel(int x, int y, short int colour) {
    if (y < 0 || y >= SCREEN_H || x < 0 || x >= SCREEN_W) return;
    *(pixel_buffer_start + (y << 9) + x) = colour;
}

void fill_background(short int colour) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            plot_pixel(x, y, colour);
}

void plot_scaled_pixel(int x, int y, short int colour, float scale) {
    for (int dy = 0; dy < (int)scale; dy++)
        for (int dx = 0; dx < (int)scale; dx++)
            plot_pixel(x+dx, y+dy, colour);
}

void draw_char(int x, int y, char c, short int fill, short int outline, float scale) {
    for (int row = 0; row < FONT_H; row++) {
        unsigned char bits = FONT[(unsigned char)c][row];
        for (int col = 0; col < FONT_W; col++) {
            if (bits & (1 << (FONT_W - col - 1))) {
                int px = x + (int)(col * scale);
                int py = y + (int)(row * scale);
                plot_scaled_pixel(px-(int)scale, py, outline, scale);
                plot_scaled_pixel(px+(int)scale, py, outline, scale);
                plot_scaled_pixel(px, py+(int)scale, outline, scale);
                plot_scaled_pixel(px, py-(int)scale, outline, scale);
                plot_scaled_pixel(px-(int)scale, py-(int)scale, outline, scale);
                plot_scaled_pixel(px+(int)scale, py-(int)scale, outline, scale);
                plot_scaled_pixel(px+(int)scale, py+(int)scale, outline, scale);
                plot_scaled_pixel(px-(int)scale, py+(int)scale, outline, scale);
            }
        }
    }
    for (int row = 0; row < FONT_H; row++) {
        unsigned char bits = FONT[(unsigned char)c][row];
        for (int col = 0; col < FONT_W; col++) {
            if (bits & (1 << (FONT_W - col - 1))) {
                int px = x + (int)(col * scale);
                int py = y + (int)(row * scale);
                plot_scaled_pixel(px,   py,   fill, scale);
                plot_scaled_pixel(px-1, py,   fill, scale);
                plot_scaled_pixel(px,   py-1, fill, scale);
            }
        }
    }
}

void draw_letters(int x, int y, const char *s, short int fill, short int outline, float scale) {
    while (*s) {
        draw_char(x, y, *s, fill, outline, scale);
        x += (int)(scale * (FONT_W + 1));
        s++;
    }
}

void draw_letters_centered(int y, const char *s, short int fill, short int outline, float scale) {
    if (!s || scale <= 0) return;
    int w = (int)(strlen(s) * scale * (FONT_W + 1));
    draw_letters((SCREEN_W - w) / 2, y, s, fill, outline, scale);
}

void draw_circle(int x, int y, int r, short int colour) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                plot_pixel(x+dx, y+dy, colour);
}

void draw_arc_highlight(int x, int y, int r, float c_deg, float s_deg,
                        int m_thick, short int colour) {
    float st  = c_deg - s_deg / 2.0f;
    float end = c_deg + s_deg / 2.0f;
    for (float deg = st; deg <= end; deg += 1.5f) {
        float angle    = DEG2RAD(deg);
        float distance = fabs(deg - c_deg) / (s_deg / 2.0f);
        int thick = (int)(m_thick * (1.0f - distance));
        if (thick < 1) thick = 1;
        for (int i = 0; i < thick; i++)
            plot_pixel(x + (int)((r+i)*cos(angle)),
                       y - (int)((r+i)*sin(angle)), colour);
    }
}

void draw_disc(int x, int y, int r, bool show_button, float th,
               volatile int *mouse_ptr, bool echo, bool blend, bool multi) {
    draw_circle(x, y, 65, 0x0000);
    draw_circle(x, y, 60, 0x2104);
    draw_circle(x, y, 52, 0x39E7);
    draw_circle(x, y, 44, 0x4208);
    draw_circle(x, y, 36, 0x2104);

    if (show_button) {
        draw_arc_highlight(x, y, r-4,  th,       48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, th,       42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, th,       36, 3, 0x4208);
        draw_arc_highlight(x, y, r-4,  th+120,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, th+120,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, th+120,   36, 3, 0x4208);
        draw_arc_highlight(x, y, r-4,  th+240,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-10, th+240,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-16, th+240,   36, 3, 0x4208);
        draw_enter_button(x, y, r*1.2f, r*0.38f, 0);

    } else if (echo) {
        draw_arc_highlight(x, y, r-1,  th,       48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th,       42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th,       36, 3, 0x4208);
        draw_arc_highlight(x, y, r-1,  th+120,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th+120,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th+120,   36, 3, 0x4208);
        draw_arc_highlight(x, y, r-1,  th+240,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th+240,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th+240,   36, 3, 0x4208);
        draw_arc_highlight(x, y, r-6,  th-30,    32, 4, 0xAD55);
        draw_arc_highlight(x, y, r-14, th-30,    26, 3, 0x6B4D);
        draw_arc_highlight(x, y, r-22, th-30,    20, 2, 0x3186);
        draw_arc_highlight(x, y, r-6,  th-30+120,32, 4, 0xAD55);
        draw_arc_highlight(x, y, r-14, th-30+120,26, 3, 0x6B4D);
        draw_arc_highlight(x, y, r-22, th-30+120,20, 2, 0x3186);
        draw_arc_highlight(x, y, r-6,  th-30+240,32, 4, 0xAD55);
        draw_arc_highlight(x, y, r-14, th-30+240,26, 3, 0x6B4D);
        draw_arc_highlight(x, y, r-22, th-30+240,20, 2, 0x3186);
        draw_arc_highlight(x, y, r-12, th-60,    20, 3, 0x8410);
        draw_arc_highlight(x, y, r-20, th-60,    16, 2, 0x4208);
        draw_arc_highlight(x, y, r-28, th-60,    12, 1, 0x2104);
        draw_arc_highlight(x, y, r-12, th-60+120,20, 3, 0x8410);
        draw_arc_highlight(x, y, r-20, th-60+120,16, 2, 0x4208);
        draw_arc_highlight(x, y, r-28, th-60+120,12, 1, 0x2104);
        draw_arc_highlight(x, y, r-12, th-60+240,20, 3, 0x8410);
        draw_arc_highlight(x, y, r-20, th-60+240,16, 2, 0x4208);
        draw_arc_highlight(x, y, r-28, th-60+240,12, 1, 0x2104);

    } else if (blend) {
        for (int i = 0; i < 6; i++) {
            float t = th + i * 60;
            draw_arc_highlight(x, y, r-1,  t, 64, 5, 0xC618);
            draw_arc_highlight(x, y, r-7,  t, 58, 4, 0x8410);
            draw_arc_highlight(x, y, r-13, t, 52, 3, 0x4208);
        }
    } else if (multi) {
        for (int i = 0; i < 6; i++) {
            float t = th + i * 60;
            draw_arc_highlight(x, y, r-1,  t, 64, 5, 0xC618);
            draw_arc_highlight(x, y, r-7,  t, 58, 4, 0x8410);
            draw_arc_highlight(x, y, r-13, t, 52, 3, 0x4208);
            draw_arc_highlight(x, y, r-19, t, 44, 4, 0xC618);
            draw_arc_highlight(x, y, r-25, t, 38, 3, 0x8410);
            draw_arc_highlight(x, y, r-31, t, 32, 2, 0x4208);
            draw_arc_highlight(x, y, r-37, t, 22, 3, 0xC618);
            draw_arc_highlight(x, y, r-43, t, 18, 2, 0x8410);
            draw_arc_highlight(x, y, r-49, t, 12, 1, 0x4208);
        }
    } else {
        draw_arc_highlight(x, y, r-1,  th,       48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th,       42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th,       36, 3, 0x4208);
        draw_arc_highlight(x, y, r-1,  th+120,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th+120,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th+120,   36, 3, 0x4208);
        draw_arc_highlight(x, y, r-1,  th+240,   48, 5, 0xC618);
        draw_arc_highlight(x, y, r-7,  th+240,   42, 4, 0x8410);
        draw_arc_highlight(x, y, r-13, th+240,   36, 3, 0x4208);
    }
}

void draw_enter_button(int x, int y, int w, int h, bool changed) {
    int x0 = x - w/2, y0 = y - h/2;
    draw_rectangle(x0, y0, w, h, 0x4208);
    draw_rectangle(x0+2, y0+2, w-4, h-4, WHITE);
    if (changed) {
        draw_rectangle(x0+3, y0+3, w-6, h-6, GREEN);
    }
    else {
        draw_rectangle(x0+3, y0+3, w-6, h-6, GREY);
    }
    draw_letters_box(SCREEN_W/2.12f, SCREEN_H/2.305f, w, h, "ENTER", WHITE, BLACK, 1.5f);
    
}

void draw_rectangle(int x, int y, int w, int h, short int colour) {
    for (int dh = 0; dh <= h; dh++)
        for (int dw = 0; dw <= w; dw++)
            plot_pixel(x+dw, y+dh, colour);
}

void draw_letters_box(int x, int y, int w, int h, const char *s,
                      short int fill, short int outline, float scale) {
    int length  = strlen(s);
    int l_width = (length-1) * length * FONT_W;
    int l_height= (int)(scale * FONT_H);
    draw_letters(x + (w - l_width)/2, y + (y - l_height)/2, s, fill, outline, scale);
}

void draw_line(int x0, int y0, int x1, int y1, short int colour) {
    bool steep = abs(y1-y0) > abs(x1-x0);
    if (steep)  { swap(&x0,&y0); swap(&x1,&y1); }
    if (x0>x1)  { swap(&x0,&x1); swap(&y0,&y1); }
    int dx = x1-x0, dy = abs(y1-y0);
    int err = -(dx/2), y = y0, ys = (y0<y1)?1:-1;
    for (int x = x0; x <= x1; x++) {
        if (steep) plot_pixel(y, x, colour);
        else       plot_pixel(x, y, colour);
        err += dy;
        if (err > 0) { y += ys; err -= dx; }
    }
}

void draw_exit_button(int x0, int y0, int w, int h,
                      short int lc, short int oc, short int ic, int sc) {
    draw_rectangle(x0-sc, y0-sc, w+(2*sc), h+(2*sc), oc);
    draw_rectangle(x0, y0, w, h, ic);
    draw_line(x0,   y0-2, x0+w+2, y0+w, lc); draw_line(x0,   y0-1, x0+w+1, y0+w, lc);
    draw_line(x0,   y0,   x0+w,   y0+w, lc); draw_line(x0,   y0+1, x0+w-1, y0+w, lc);
    draw_line(x0,   y0+2, x0+w-2, y0+w, lc); draw_line(x0+w, y0-2, x0-2,   y0+w, lc);
    draw_line(x0+w, y0-1, x0-1,   y0+w, lc); draw_line(x0+w, y0,   x0,     y0+w, lc);
    draw_line(x0+w, y0+1, x0+1,   y0+w, lc); draw_line(x0+w, y0+2, x0+2,   y0+w, lc);
}

void draw_push_button(int x, int y, int w, int h, int r,
                      short int oc, short int ic, int sc, int mode, short int on) {
    draw_rectangle(x, y, w, h, oc);
    draw_rectangle(x+sc, y+sc, w-(2*sc), h-(2*sc), ic);
    draw_circle(x+(w/2), y+(w/2), r, mode ? on : oc);
    draw_circle(x+(w/6),   y+(h/6),   w/15, oc);
    draw_circle(x+w-(w/6), y+(h/6),   w/15, oc);
    draw_circle(x+(w/6),   y+h-(h/6), w/15, oc);
    draw_circle(x+w-(w/6), y+h-(h/6), w/15, oc);
}

void draw_switch(int x, int y, int w, int h,
                 short int base, short int off, short int on,
                 int sx, int sy, int mode) {
    draw_rectangle(x, y, w, h, base);
    draw_rectangle(x+sx, y+(sy*2), w-(sx*2), h-(sy*4), off);
    int ky = mode ? y+(sy*2) : y+(h/2);
    draw_rectangle(x+sx, ky, w-(sx*2), ((h-(sy*4))/2)+1, on);
}

void draw_play_button(int x, int y_ctr, int h, int w, short int colour) {
    for (int y = 0; y < h; y++) {
        int y_off = y - h/2;
        int bw = w * ((h/2) - abs(y_off)) / (h/2);
        for (int x0 = 0; x0 < bw; x0++)
            plot_pixel(x+x0, y_ctr+y_off, colour);
    }
}

void draw_led(int x, int y, int w, int h,
              short int outer, short int inner, int sc, int mode, short int on) {
    draw_rectangle(x, y, w, h, outer);
    draw_rectangle(x+sc, y+sc, w-(sc*2), h-(sc*2), mode ? on : inner);
}

void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }

void wait_for_vsync(void) {
    volatile int *p = (int *)FB_ADDR;
    *p = 1;
    int s = *(p + 3);
    while (s & 0x01) {
        s = *(p + 3);
        /* Audio fills automatically via IRQ 18 during vsync wait */
    }
}
