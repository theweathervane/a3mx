/* Screen fades: small tasks (task slot 3) that step a fade level once a frame until it is complete,
 * then end. A screen starts one and watches FADE_RUNNING go back to 0.
 *
 *   fade in / out by a float level (0 .. 1, 1/16 a frame): 0x0880AB04 up, 0x0880AC18 down; the level
 *     is drawn as a full-screen colour (set with 0x089E0BD8: black)
 *   fade by an alpha (0 .. 255, 15 a frame): 0x0880AD60 up, 0x0880ADF0 down: not decompiled yet, as
 *     no test scenario reaches them (their starters, 0x0880ACE8 / 0x0880AD24, have no known caller)
 *
 * FADE_TASK is set while a fade task holds slot 3, so a second request is ignored.
 */
#include "../decomp.h"
#include "task.h"
#include <string.h>

#define GAME           0x08E75910u
#define FADE_RUNNING   (GAME + 0x5C)   /* s8: -1 while a fade steps; the fade sets 0 when complete (others may stop it early) */
#define FADE_TASK      (GAME + 0x5D)   /* s8: -1 while a fade task holds slot 3 */
#define FADE_UPDATED   0x08E75B9Au     /* u16: set to 1 each step */
#define FADE_LEVEL     0x08E75B9Cu     /* float: 0 clear .. 1 full colour */
#define FADE_SLOT      3
#define FADE_STEP      (1.0f / 16)

#define FADE_SETTING   0x08E75C5Cu     /* u32: 1 once 0x0880AA54 has set the two below */
#define FADE_SETTING_A 0x08E75C60u
#define FADE_SETTING_B 0x08E75C64u

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t fade_running(void) { return (int8_t)*B(FADE_RUNNING); }
static inline float level(void) { float f; memcpy(&f, guest_ptr(FADE_LEVEL), 4); return f; }
static inline void set_level(float f) { memcpy(guest_ptr(FADE_LEVEL), &f, 4); }

void f_089E0BD8(Cpu *c);   /* graphics: the fade colour (0xAABBGGRR) */
void f_088D7F28(Cpu *c);   /* non-zero: no fade now (network) */
void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_08805530(Cpu *c);   /* task.c: the task ends */
void f_088054C8(Cpu *c);   /* task.c: start a task */

/* The fade task is done: its slot is free again. */
static void fade_task_end(Cpu *c) {
    *B(FADE_TASK) = 0;
    CALL0(f_08805530);
}

/* Fades to the colour: the level goes up from 0 to 1. */
static void fade_level_up(Cpu *c) {
    *B(FADE_RUNNING) = 0xFF;
    set_level(0.0f);
    CALL1(f_089E0BD8, 0xFF000000u);
    if (fade_running() != 0) {
        float f = level();
        do {
            set_level(f + FADE_STEP);
            if (level() <= 1.0f) {
                *H(FADE_UPDATED) = 1;
            } else {
                *B(FADE_RUNNING) = 0;
                set_level(1.0f);
                *H(FADE_UPDATED) = 1;
            }
            CALL1(f_088054F0, 1);
            f = level();
        } while (fade_running() != 0);
    }
    fade_task_end(c);
}

/* Fades from the colour: the level goes down from 1 to 0. */
static void fade_level_down(Cpu *c) {
    *B(FADE_RUNNING) = 0xFF;
    set_level(1.0f);
    CALL1(f_089E0BD8, 0xFF000000u);
    if (fade_running() != 0) {
        float f = level();
        do {
            set_level(f - FADE_STEP);
            if (!(level() < 0.0f)) {
                *H(FADE_UPDATED) = 1;
            } else {
                set_level(0.0f);
                *B(FADE_RUNNING) = 0;
                *H(FADE_UPDATED) = 1;
            }
            CALL1(f_088054F0, 1);
            f = level();
        } while (fade_running() != 0);
    }
    fade_task_end(c);
}

/* Starts `fade` in slot 3 unless a fade task is already there. */
static void start_fade(gaddr fade) {
    if ((int8_t)*B(FADE_TASK) != 0) return;
    *B(FADE_TASK) = 0xFF;
    COVERAGE(088054C8);
    Task *t = task_slot(FADE_SLOT);
    t->entry = fade;
    t->state = TASK_START;
}

/* Starts a fade to the colour, unless the network says not now; returns what it said. */
static uint32_t start_fade_level_up(Cpu *c) {
    uint32_t busy = CALL0(f_088D7F28);
    if (busy == 0) start_fade(0x0880AB04);
    return busy;
}

/* ---------------------------------------------------------------- adapters (frames as the originals') */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
/* fade tasks (entries started by the functions below) */
SFA_FUNC(0880AB04, game_fade_level_up)   { FRAME(0x30, fade_level_up(c)); }
SFA_FUNC(0880AC18, game_fade_level_down) { FRAME(0x30, fade_level_down(c)); }
/* int start_fade_out(void): v0 is the network check's answer, as the original leaves it */
SFA_FUNC(0880AA74, game_start_fade_out)  { uint32_t r; FRAME(0x20, r = start_fade_level_up(c)); RETURN(r); }
/* void start_fade_in(void) (v0 left as it was) */
SFA_FUNC(0880ABD4, game_start_fade_in)   { start_fade(0x0880AC18); }
/* void fade_setting(a, b) */
SFA_FUNC(0880AA54, game_fade_setting)    { *W(FADE_SETTING_A) = ARG(0); *W(FADE_SETTING_B) = ARG(1); *W(FADE_SETTING) = 1; }
