/* The two ways the game queues a sound cue (0x0880A0BC): plain, and from an object with its volume by
   side and its pan from where it stands on screen. */
#include "step.h"
#include "../guest_mem.h"

void f_0880A0BC(Cpu *c);

#define BATTLE       0x092E1228u
#define CAMERA_LAYER 0x092E88A0u
#define SIDE_BANKS   0x092E89E8u   /* a sound bank per side */
#define SIDE_VOLUMES 0x08A22D20u   /* s8, four per setting (BATTLE +56) */

static void queue(Cpu *c, uint32_t bank, uint32_t vol, uint32_t cue, uint32_t pan, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = bank; ARG(1) = vol; ARG(2) = cue; ARG(3) = pan;
    c->r[31] = back; f_0880A0BC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (volume, cue, pan): the cue queued from bank 0. */
SFA_FUNC(0887C4CC, sound_cue) {
    queue(c, 0, ARG(0), ARG(1) & 0xFFFF, (uint32_t)(int32_t)(int16_t)ARG(2), 0x0887C4ECu);
}

/* (object, side, cue): the cue queued with the side's volume for the current setting, panned by the
   object's x against the camera (+18, held to -192 .. 575); cues of group 2 and up come from the side's
   own bank. */
SFA_FUNC(0887C408, sound_cue_from) {
    gaddr o = ARG(0);
    uint32_t side = ARG(1), cue = ARG(2);
    int32_t x = (int16_t)(S16(o + 18) - S16(CAMERA_LAYER + 18));
    int32_t vol = (int8_t)*B(SIDE_VOLUMES + ((uint32_t)S16(BATTLE + 56) << 2) + side);
    int32_t grp = (int32_t)(cue & 0xFF00) >> 8;
    if (x < -192) x = -192;
    else if (x >= 576) x = 0x23F;
    uint32_t bank = grp < 2 ? 0 : *W(SIDE_BANKS + (side << 2));
    c->r[8] = cue & 0xFFFF;
    queue(c, bank, (uint32_t)vol, cue & 0xFFFF, (uint32_t)x, 0x0887C4C0u);
}
