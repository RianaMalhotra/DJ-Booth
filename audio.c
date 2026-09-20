/*
 * audio_m1.c
 * Milestone 1 - Audio Output: Pre-recorded Playback + Volume Control
 *
 * Features:
 *   1. Plays pre-recorded audio from audio_data.h (PinkPantheress - Stateside)
 *   2. Volume control via SW[9:7]  -> 8 levels (0 = mute, 7 = full)
 *   3. KEY0 = restart playback from beginning
 *      KEY1 = pause / resume toggle
 *   4. LED[6:0] = volume bar display
 *      LED[9]   = lit when paused
 */

#include "audio_data.h" /* provides: audio_samples[], AUDIO_LEN */

/* Memory-mapped peripheral base addresses
 */
#define AUDIO_BASE 0xFF203040  /* WM8731 Audio CODEC core            */
#define SW_BASE 0xFF200040     /* 10 slide switches                  */
#define KEY_BASE 0xFF200050    /* Push-button data                   */
#define KEY_EDGECAP 0xFF20005C /* Push-button edge-capture register  */
#define LED_BASE 0xFF200000    /* Red LEDs                           */

/* Audio peripheral register struct (from part2.c) */
struct audio_t {
  volatile unsigned int control; /* control / status register          */
  volatile unsigned char rarc;   /* right audio input  FIFO count      */
  volatile unsigned char ralc;   /* left  audio input  FIFO count      */
  volatile unsigned char wsrc;   /* right audio output FIFO space      */
  volatile unsigned char wslc;   /* left  audio output FIFO space      */
  volatile unsigned int ldata;   /* left  data register (24-bit audio) */
  volatile unsigned int rdata;   /* right data register (24-bit audio) */
};

/* Peripheral pointers
 */
static struct audio_t* const audiop = (struct audio_t*)AUDIO_BASE;
static volatile int* const SW_ptr = (volatile int*)SW_BASE;
static volatile int* const EDGE_ptr = (volatile int*)KEY_EDGECAP;
static volatile int* const LED_ptr = (volatile int*)LED_BASE;

/* Volume control
 */
/*  SW[9:7] = 3 bits -> 8 levels (0 = mute, 7 = full volume) */
#define VOL_MASK 0x380 /* bits 9,8,7 of SW register */
#define VOL_SHIFT 7

/* Read SW[9:7] and return volume level 0-7 */
static inline int get_volume_level(void) {
  return ((*SW_ptr) & VOL_MASK) >> VOL_SHIFT;
}

/* Scale a 24-bit signed audio sample by volume level (0-7) */
static inline int apply_volume(int sample, int level) {
  if (level == 0) return 0;      /* mute                  */
  if (level == 7) return sample; /* full — no scaling     */
  return (sample / 7) * level;   /* linear scale          */
}

/* LED volume bar
 */
/*  level 0 → no LEDs, level 7 → LED[6:0] all on */
/*  LED[9] lights up when paused */
static void update_leds(int vol_level, int paused) {
  int led_val = (1 << vol_level) - 1; /* e.g. level 3 → 0b0000111 */
  if (paused) led_val |= (1 << 9);    /* top LED on when paused   */
  *LED_ptr = led_val;
}

/* KEY edge-capture handler
 */
/*  Returns: 1 if KEY0 pressed, 2 if KEY1 pressed, 0 if nothing */
static int check_keys(void) {
  int edge = *EDGE_ptr;
  if (edge & 0x1) {
    *EDGE_ptr = edge; /* clear edge-capture register */
    return 1;         /* KEY0 pressed                */
  }
  if (edge & 0x2) {
    *EDGE_ptr = edge;
    return 2; /* KEY1 pressed                */
  }
  return 0;
}

/* Audio CODEC init: flush FIFOs
 */
static void audio_init(void) {
  audiop->control = 0x8; /* assert FIFO clear bit */
  audiop->control = 0x0; /* release               */
}

/*
 * main
 *
 */
int main(void) {
  int vol;
  int sample_l, sample_r;
  int playback_index = 0; /* current position in audio_samples[]  */
  int paused = 0;         /* 0 = playing, 1 = paused              */
  int key;

  /* Initialise audio CODEC */
  audio_init();

  while (1) {
    /* Check KEY presses (non-blocking edge-capture)  */
    key = check_keys();

    if (key == 1) { /* KEY0 - restart from beginning     */
      playback_index = 0;
      paused = 0;
    } else if (key == 2) { /* KEY1 → toggle pause/resume        */
      paused = !paused;
    }

    /*  Read volume from SW[9:7] and update LED bar  */
    vol = get_volume_level();
    update_leds(vol, paused);

    /*  If paused, skip audio output  */
    if (paused) continue;

    /*  Wait until output FIFO has space (from part3.c)  */
    while (audiop->wsrc == 0); /* wait for right channel FIFO space */
    while (audiop->wslc == 0); /* wait for left  channel FIFO space */

    /*  Get next sample from pre-recorded audio array  */
    sample_l = audio_samples[playback_index];
    sample_r = audio_samples[playback_index];

    /*  Apply volume scaling  */
    sample_l = apply_volume(sample_l, vol);
    sample_r = apply_volume(sample_r, vol);

    /*  Write to CODEC output FIFO  */
    audiop->ldata = (unsigned int)sample_l;
    audiop->rdata = (unsigned int)sample_r;

    /* Advance index, wrap around at end */
    playback_index = (playback_index + 1) % AUDIO_LEN;
  }

  return 0;
}
