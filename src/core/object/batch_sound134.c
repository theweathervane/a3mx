/* Sound-cue callers, 42 - 51 instructions: a menu's blinking cursor with its choice change cue, effects
   spawned at a random offset with their cue, an animation frame's sound, and the cues of being hit.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088215C4(Cpu *c); void f_088969BC(Cpu *c); void f_0889C9FC(Cpu *c); void f_0889D048(Cpu *c);
void f_0889CD8C(Cpu *c); void f_08821368(Cpu *c); void f_08822EAC(Cpu *c); void f_0887C408(Cpu *c);
void f_0886ABB4(Cpu *c); void f_0886E3CC(Cpu *c);

#define GAME 0x08E75910u

static void side_cue(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = n;
    c->r[31] = back; f_0887C408(c);
}

/* The menu's cursor blink (+82: low byte the frame 0 - 3, high byte the wait): every 6 ticks the next
   frame is drawn (0x088969BC sprite 0x138 + frame, palette the side's (+337) + 0x14). The wait and frame
   are kept in a halfword on the stack as the original does. */
static void cursor_blink(Cpu *c, gaddr m, gaddr side_of, gaddr sp, uint32_t back) {
    *H(sp + 16) = (uint16_t)S16(m + 82);
    uint32_t wait = (uint32_t)*B(sp + 17) - 1;
    *B(sp + 17) = (uint8_t)wait;
    if ((wait & 0xFF) == 0) {
        uint32_t frame = ((uint32_t)*B(sp + 16) + 1) & 3;
        *B(sp + 16) = (uint8_t)frame;
        *B(sp + 17) = 6;
        ARG(0) = frame + 0x138; ARG(1) = (uint32_t)(S8(side_of + 337) + 0x14); ARG(2) = 1;
        c->r[31] = back; f_088969BC(c);
    }
    *H(m + 82) = *H(sp + 16);
}

/* (menu, item): the cursor blinks; when the item's choice (+438) is not the menu's (+79) the cursor cue
   plays and 0x0889C9FC takes it; then 0x088215C4. */
SFA_FUNC(0889C954, menu_cursor_item) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s1;
    c->r[17] = m;
    *W(sp + 20) = s0; *W(sp + 28) = ra;
    c->r[16] = it;
    cursor_blink(c, m, it, sp, 0x0889C9B4u);
    int32_t n = S8(it + 438);
    c->r[16] = (uint32_t)n;
    if ((uint32_t)n != *B(m + 79)) {
        ARG(0) = m; c->r[31] = 0x0889C9D4u; f_0889D048(c);
        ARG(0) = m; ARG(1) = (uint32_t)n; c->r[31] = 0x0889C9E0u; f_0889C9FC(c);
    }
    ARG(0) = m; c->r[31] = 0x0889C9E8u; f_088215C4(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu, owner): as above, the choice being the game's +20 low byte, taken by 0x0889CD8C. */
SFA_FUNC(0889CCD4, menu_cursor_game) {
    gaddr m = ARG(0), ow = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0;
    c->r[16] = m;
    *W(sp + 24) = s1; *W(sp + 28) = ra;
    cursor_blink(c, m, ow, sp, 0x0889CD38u);
    uint32_t n = (uint32_t)S16(GAME + 20) & 0xFF;
    c->r[17] = n;
    if (n != *B(m + 79)) {
        ARG(0) = m; c->r[31] = 0x0889CD64u; f_0889D048(c);
        ARG(0) = m; ARG(1) = n; c->r[31] = 0x0889CD70u; f_0889CD8C(c);
    }
    ARG(0) = m; c->r[31] = 0x0889CD78u; f_088215C4(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): effect 0x3B spawned (0x08822EAC) at a random one of 16 offsets (0x08A70B48) from the owner,
   tied to it (+88), +9 = 3, +72 = 1, +78 = 0x52, +86 / +164 copied, with cue 0x220 from the owner's
   fighter's side. */
SFA_FUNC(0893A7E4, effect_3b_random_cue) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    ARG(0) = 0x3B; ARG(1) = 1;
    c->r[31] = 0x0893A804u; f_08822EAC(c);
    gaddr e = c->r[2];
    c->r[17] = e;
    ARG(0) = 3;
    if (e != 0) {
        *B(e + 9) = 3; *B(e + 72) = 1;
        c->r[31] = 0x0893A820u; f_08821368(c);
        gaddr off = 0x08A70B48u + ((c->r[2] & 0xF) << 2);
        *H(e + 18) = (uint16_t)(S16(o + 18) + S16(off));
        *H(e + 22) = (uint16_t)(S16(o + 22) + S16(off + 2));
        *B(e + 78) = 0x52;
        *W(e + 88) = o;
        *H(e + 86) = *H(o + 86);
        *W(e + 164) = *W(o + 164);
        ARG(0) = e; ARG(1) = (uint32_t)(int32_t)S8(*W(o + 88) + 337); ARG(2) = 0x220;
        c->r[31] = 0x0893A884u; f_0887C408(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (part): the animation step (+28) read: its frame (+356 + 12 * step +8) kept at +296, its four bytes at
   +74 - +77, the frame's picture (+200 + 4 * frame +0) at +256 / +204; a frame sound (+2) that the
   owner fighter (+88) maps (+300 entry +2, then its cue list +292) plays from the fighter's side. */
SFA_FUNC(088D3CD8, anim_frame_sound) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr a = *W(o + 28);
    gaddr fr = *W(o + 356) + (uint32_t)*H(a + 8) * 12;
    *W(o + 296) = fr;
    *B(o + 74) = *B(a);
    *B(o + 75) = *B(a + 1);
    *B(o + 76) = *B(a + 2);
    uint8_t b3 = *B(a + 3);
    *B(o + 77) = b3;
    gaddr pic = *W(o + 200) + ((uint32_t)*B(fr) << 2);
    *W(o + 256) = pic; *W(o + 204) = pic;
    uint32_t k = *B(fr + 2);
    *W(c->r[29] + 16) = ra;
    if (k != 0) {
        gaddr f = *W(o + 88);
        uint32_t v = *B(*W(f + 300) + (k << 2) + 2);
        if (v != 0 && *W(f + 292) != 0) {
            int32_t n = S16(*W(f + 292) + v + v);
            if (n != 0) side_cue(c, f, (uint32_t)n, 0x088D3D88u);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (attacker, fighter, hit): the cues of being hit: 0x2B unless the hit's +15 has bit 7, 0x2E (and
   0x0886E3CC 0, 1) with the fighter's +703, 0x217 in state 0x26 with +820 and +236 = 4; then 0x0886ABB4
   and 0x0886E3CC (attacker, 9, 8). */
SFA_FUNC(0886AAE8, hit_cues) {
    gaddr a = ARG(0), f = ARG(1), hit = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    uint32_t h15 = *B(hit + 15);
    *W(c->r[29] + 20) = s1;
    c->r[17] = a;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if ((h15 & 0x80) == 0) side_cue(c, f, 0x2B, 0x0886AB1Cu);
    if (S8(f + 703) != 0) {
        side_cue(c, f, 0x2E, 0x0886AB38u);
        ARG(0) = f; ARG(1) = 0; ARG(2) = 1;
        c->r[31] = 0x0886AB48u; f_0886E3CC(c);
    }
    if (S8(f + 338) == 0x26 && *W(f + 820) != 0 && S8(f + 236) == 4) side_cue(c, f, 0x217, 0x0886AB84u);
    ARG(0) = a; ARG(1) = f; c->r[31] = 0x0886AB90u; f_0886ABB4(c);
    ARG(0) = a; ARG(1) = 9; ARG(2) = 8; c->r[31] = 0x0886ABA0u; f_0886E3CC(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
