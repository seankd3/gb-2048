#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/hardware.h>

#include "sound.h"

/* Frequency register values, from reg = 2048 - 131072/Hz.
   The scale is pentatonic on purpose: merges cascade and overlap, and a
   pentatonic set has no interval that can clash. */
#define N_C4 1547
#define N_E4 1650
#define N_G4 1714
#define N_A4 1750
#define N_C5 1798
#define N_D5 1825
#define N_E5 1849
#define N_G5 1881
#define N_A5 1899
#define N_C6 1923
#define N_D6 1936
#define N_E6 1949
#define N_G6 1964
#define N_A6 1974
#define N_C7 1985

typedef struct {
    uint16_t freq;
    uint8_t  frames;
} Note;

/* Channel 1's registers start at 0xFF11 and channel 2's at 0xFF16, and both
   lay out duty / envelope / freq-low / freq-high at the same four offsets, so
   a base byte addresses either channel with one branch-free write path. */
#define CH1_BASE 0x11
#define CH2_BASE 0x16

typedef struct {
    const Note *seq;
    uint8_t len, idx, timer;
    uint8_t env, duty;
    uint8_t base;        /* low byte of the channel's register block */
} Voice;

/* Channel 1 carries anything melodic (merges, fanfares). Channel 2 carries
   interface blips, so a merge and a move can sound together without either
   cutting the other off. */
static Voice v1, v2;

/* Merge pitch is built per call, so this needs to outlive the trigger. */
static Note merge_seq[2];

static const Note seq_move[]   = { {N_A4,  2} };
static const Note seq_reject[] = { {N_C4,  4} };
static const Note seq_start[]  = { {N_C5,  4}, {N_G5, 6} };
static const Note seq_win[]    = { {N_C5,  5}, {N_E5, 5}, {N_G5, 5}, {N_C6, 14} };
static const Note seq_over[]   = { {N_G4,  9}, {N_E4, 9}, {N_C4, 20} };

/* Indexed by the exponent a merge produced, so 4 is low and 2048 is a high C.
   Building something big literally sounds bigger. */
static const uint16_t merge_pitch[18] = {
    N_C4, N_C4, N_C4, N_E4, N_G4, N_A4, N_C5, N_D5, N_E5,
    N_G5, N_A5, N_C6, N_D6, N_E6, N_G6, N_A6, N_C7, N_C7
};

static void chan_write(uint8_t base, uint16_t freq, uint8_t env, uint8_t duty) {
    volatile uint8_t *r = (volatile uint8_t *)(0xFF00u + base);
    r[0] = duty;
    r[1] = env;
    r[2] = (uint8_t)(freq & 0xFF);
    r[3] = (uint8_t)(0x80 | (freq >> 8));   /* trigger */
}

static void chan_off(uint8_t base) {
    volatile uint8_t *r = (volatile uint8_t *)(0xFF00u + base);
    r[1] = 0x00;    /* zero the envelope: switches the DAC off */
    r[3] = 0x80;
}

static void voice_play(Voice *v, const Note *seq, uint8_t len,
                       uint8_t env, uint8_t duty) {
    v->seq   = seq;
    v->len   = len;
    v->idx   = 0;
    v->env   = env;
    v->duty  = duty;
    v->timer = seq[0].frames;
    chan_write(v->base, seq[0].freq, env, duty);
}

static void voice_update(Voice *v) {
    if (v->seq == 0) return;

    v->timer--;
    if (v->timer != 0) return;

    v->idx++;
    if (v->idx >= v->len) {
        v->seq = 0;
        chan_off(v->base);
        return;
    }
    v->timer = v->seq[v->idx].frames;
    chan_write(v->base, v->seq[v->idx].freq, v->env, v->duty);
}

void sound_init(void) {
    /* Master enable has to come first: writes to the other sound registers
       are ignored while the APU is off. */
    NR52_REG = 0x80;
    NR51_REG = 0xFF;    /* every channel to both outputs */
    NR50_REG = 0x77;    /* full volume, both sides */
    NR10_REG = 0x00;    /* channel 1 sweep off, set once and left alone */

    v1.base = CH1_BASE; v1.seq = 0;
    v2.base = CH2_BASE; v2.seq = 0;
}

void sound_update(void) {
    voice_update(&v1);
    voice_update(&v2);
}

/* Duty 0x00 is the 12.5% pulse, which is thin and soft rather than bright,
   and volume 3 keeps it well under the merge. */
void sound_move(void) {
    voice_play(&v2, seq_move, 1, 0x31, 0x00);
}

void sound_merge(uint8_t exp) {
    uint16_t p;

    if (exp > 17) exp = 17;
    p = merge_pitch[exp];

    merge_seq[0].freq   = p;
    merge_seq[0].frames = 3;
    merge_seq[1].freq   = (p + 40u > 2047u) ? 2047u : (uint16_t)(p + 40u);
    merge_seq[1].frames = 5;

    voice_play(&v1, merge_seq, 2, 0xA2, 0x80);
}

void sound_reject(void) {
    voice_play(&v2, seq_reject, 1, 0x52, 0x40);
}

void sound_start(void) {
    voice_play(&v2, seq_start, 2, 0x82, 0x80);
}

void sound_win(void) {
    voice_play(&v1, seq_win, 4, 0xC2, 0x80);
}

void sound_gameover(void) {
    voice_play(&v1, seq_over, 3, 0x93, 0x80);
}

