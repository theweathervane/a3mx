/* Steps of 78 - 80 instructions: a copy spawned on an end frame (0x088F00E4), a win-pose start with a
   special for a locked opponent in stage 9 (0x08900EFC), a lunge that steps on by its mark into a
   charge move (0x08916578) and an intro that rides its partner (0x0898E608). Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_08822D2C(Cpu *c); void f_08822EAC(Cpu *c); void f_08917870(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void stand(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
    call2(c, o, 0, f_0886163C, back);
}

/* (fighter): ticked; ended: standing; on the end frame (+75 = 0) a frame flag spawns a copy (0x08822D2C
   0 / 2) with the fighter's picture, palette, place and side: +346 = strength + 3, +112 = strength +
   2, +255 = 3, +347 = 1, +260 from 0x08A43B34, +311 = 2, +312 = 8; kept at +228 and counted (+536). */
SFA_FUNC(088F00E4, move_end_frame_copy) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088F00F8u);
    int32_t e75 = S8(o + 75);
    if (e75 < 0) call1(c, o, f_0886469C, 0x088F010Cu);
    else if (e75 == 0 && S8(o + 77) != 0) {
        ARG(0) = o; ARG(1) = 0; ARG(2) = 2; c->r[31] = 0x088F0134u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 122) = *B(o + 122); *B(e + 81) = *B(o + 81); *B(e + 121) = *B(o + 121);
            *B(e + 346) = (uint8_t)(S8(o + 194) + 3);
            *H(e + 112) = (uint16_t)(S8(o + 194) + 2);
            *B(e + 255) = 3; *B(e + 347) = 1;
            *B(e + 14) = *B(o + 14);
            *H(e + 86) = (uint16_t)S8(o + 337);
            *W(e + 164) = *W(o + 164);
            *B(e + 11) = *B(o + 11); *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
            *H(e + 38) = *H(o + 38); *H(e + 18) = *H(o + 18);
            uint32_t y = *H(o + 22);
            *W(e + 88) = o; *H(e + 22) = (uint16_t)y;
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            int32_t s = S8(o + 194);
            *B(o + 536) = (uint8_t)(n + 1);
            *B(e + 260) = (uint8_t)S8(0x08A43B34u + (uint32_t)s);
            *B(e + 311) = 2; *B(e + 312) = 8;
        }
    }
    leave(c, sv);
}

/* (fighter): a win pose: step 0: step 1, +1 = 1, and (in a match on, stage 9 unless +415 is negative,
   unlocked against a locked character 0xA) a special: effect 0x19 / 2 tied to it with +656 = 32, +372
   cleared and move 0x31; otherwise step 2 at once. Step 1 waits for the game's +6 = 2. Later standing
   once ended, else ticked. */
SFA_FUNC(08900EFC, win_pose_special) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    gaddr opp = *W(o + 92);
    if (st == 1) {
        if (*H(GAME + 6) == 2) *B(o + 7) = 2;
        leave(c, sv);
        return;
    }
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 1) = 1;
        int special = S8(MATCH + 79) == 0
                      && (S8(o + 415) < 0 || S8(GAME + 114) == 9)
                      && S8(o + 373) == 0 && S8(opp + 373) != 0 && S8(opp + 338) == 0xA;
        if (special) {
            ARG(0) = 0x19; ARG(1) = 2; c->r[31] = 0x08900F8Cu; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e != 0) { *W(e + 88) = o; *B(o + 656) = 0x20; }
            *B(o + 372) = 0;
            call2(c, o, 0x31, f_0886163C, 0x08900FB4u);
            leave(c, sv);
            return;
        }
        *B(o + 7) = 2;
    }
    if (S8(o + 75) < 0) stand(c, o, 0x08901014u);
    else call1(c, o, f_08861848, 0x08901024u);
    leave(c, sv);
}

/* (fighter): +82 counts (every 8th 0x08917870); it moves by +96; once past its mark (+0x334), when +238
   asks for its way of moving, with a pending +123, or within 64 of the opponent (32 ahead) +7 steps on
   into move 0x31 + 3 x strength + its charge (+0x338); else ticked. */
SFA_FUNC(08916578, move_lunge_charge_31) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    for (int i = 0; i < 3; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s[1];
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    uint32_t t = (uint32_t)S16(o + 82) + 1;
    *H(o + 82) = (uint16_t)t;
    *W(c->r[29] + 24) = s[2];
    gaddr m = o + 0x334;
    c->r[18] = m;
    *W(c->r[29] + 16) = s[0]; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if ((t & 7) == 0) call1(c, o, f_08917870, 0x089165B4u);
    uint32_t v = *W(o + 96);
    uint32_t f = *B(o + 11);
    *W(o + 16) = *W(o + 16) + v;
    int32_t px = S16(o + 18), mk = S16(m);
    int16_t d = (int16_t)(f != 0 ? mk - px : px - mk);
    int go = 1;
    if (d >= 0 && S8(o + 238) != ((int32_t)v < 0 ? 1 : 2) && S8(o + 123) == 0) {
        uint32_t off = f == 0 ? 0xFFE0u : 0x20u;
        int32_t r = (int32_t)((off + (uint32_t)px - (uint32_t)S16(opp + 18) + 0x40) & 0xFFFF);
        if (0x80 < r) go = 0;
    }
    if (go) {
        uint32_t st = *B(o + 7);
        int32_t sg = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t ch = S8(m + 4);
        call2(c, o, (uint32_t)(sg + sg + sg + ch + 0x31) & 0xFFFF, f_088616EC, 0x08916698u);
    } else call1(c, o, f_08861848, 0x08916650u);
    for (int i = 0; i < 3; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): intro with a partner (+600): +6 = 0: with one +6 = 1 and move 0x33, without +1 = 1, +6 =
   3 and move 0x22; +6 = 1 waits for the game's +6 = 2 (+1 = 1, +656 = 6); +6 = 2 waits for the
   partner's frame flag 2, then takes its place 32 ahead; +6 = 3 standing once ended, else ticked. */
SFA_FUNC(0898E608, intro_with_partner) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t n = *B(o + 6);
    frame_in(c, &ra);
    if (n == 0) {
        if (*W(o + 600) == 0) { *B(o + 1) = 1; *B(o + 6) = 3; call2(c, o, 0x22, f_0886163C, 0x0898E650u); }
        else { *B(o + 6) = 1; *B(o + 1) = 0; call2(c, o, 0x33, f_0886163C, 0x0898E650u); }
    } else if (n == 1) {
        if (*H(GAME + 6) == 2) { *B(o + 6) = 2; *B(o + 1) = 1; *B(o + 656) = 6; }
    } else if (n == 2) {
        gaddr q = *W(o + 600);
        if (S8(q + 77) == 2) {
            *B(o + 6) = 3;
            *H(o + 18) = *H(q + 18);
            uint32_t f = *B(o + 11);
            int32_t qy = S16(q + 22);
            int32_t x = S16(o + 18);
            *H(o + 22) = (uint16_t)qy;
            *H(o + 18) = (uint16_t)(f != 0 ? x + 0x20 : x - 0x20);
        }
    } else if (n == 3) {
        if (S8(o + 75) < 0) stand(c, o, 0x0898E728u);
        else call1(c, o, f_08861848, 0x0898E73Cu);
    }
    frame_out(c, ra);
}
