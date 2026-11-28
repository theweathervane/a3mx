/* What the last ones set free: the link's work every frame, a throw's input for four characters,
   entrances and picks by who the opponent is, a hit landing, and a few moves' steps. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define NETP      0x09523980u

void f_0888077C(Cpu *c); void f_088D60D0(Cpu *c); void f_088E51DC(Cpu *c); void f_0882FABC(Cpu *c);
void f_088E17B0(Cpu *c); void f_088E4794(Cpu *c); void f_0886469C(Cpu *c); void f_088F0BEC(Cpu *c);
void f_088F0C48(Cpu *c); void f_088F0B94(Cpu *c); void f_088F1E74(Cpu *c); void f_088616EC(Cpu *c);
void f_08861848(Cpu *c); void f_089028AC(Cpu *c); void f_0886E3CC(Cpu *c); void f_08902698(Cpu *c);
void f_0890E804(Cpu *c); void f_08821368(Cpu *c); void f_0890E400(Cpu *c); void f_0890E248(Cpu *c);
void f_08924C28(Cpu *c); void f_08924E58(Cpu *c); void f_0892540C(Cpu *c); void f_08924B90(Cpu *c);
void f_0897AA50(Cpu *c); void f_0897AA00(Cpu *c); void f_0898155C(Cpu *c); void f_0898147C(Cpu *c);
void f_089814CC(Cpu *c); void f_0886A264(Cpu *c); void f_0886C380(Cpu *c); void f_089B75A0(Cpu *c);
void f_089BDD9C(Cpu *c); void f_089C3320(Cpu *c); void f_089CA0CC(Cpu *c);
void f_0886B4D4(Cpu *c); void f_088647C4(Cpu *c); void f_08864D40(Cpu *c); void f_08864620(Cpu *c);
void f_0885C8A8(Cpu *c); void f_0885C2B4(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (object): its start: +72, +9 and state 1, +86 = 5, +12 cleared, facing against its owner, and
   straight into its step by +78 (0x0888077C). */
SFA_FUNC(08880738, object_880738_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 72) = 1; *B(o + 9) = 1; *B(o + 4) = 1;
    *H(o + 86) = 5;
    gaddr own = *W(o + 88);
    *B(o + 12) = 0;
    uint8_t facing = *B(own + 11) ^ 1;
    *W(c->r[29] + 16) = ra;
    c->r[5] = facing; c->r[6] = 5;
    *B(o + 11) = facing;
    call0(c, f_0888077C, 0x08880770u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* Both fighters' set-ups go out over the link: for each, 0x088D60D0 packs it into a buffer on the
   stack and 0x088E51DC takes it by number. Between the two, in mode 0xC or with NETP + 34 set, for
   the link kinds 2 and 4 with NETP + 36 set, fighter 2 is made a random opponent first. A 0x40
   frame keeping s0, s1; the two fighters are kept at +40 and +44 of it. */
SFA_FUNC(088D6120, link_send_fighters) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = FIGHTER_1; *W(sp + 44) = FIGHTER_2;
    *W(sp + 48) = s0; *W(sp + 52) = s1;
    c->r[16] = sp + 0x10; c->r[17] = 0;
    *W(sp + 56) = ra;
    call2(c, sp + 0x10, *W(sp + 40), f_088D60D0, 0x088D615Cu);
    call2(c, 0, sp + 0x10, f_088E51DC, 0x088D6168u);
    c->r[17] = 1;
    uint32_t kind = *B(NETP + 43);
    if ((S8(GAME + 114) == 0xC || *B(NETP + 34) != 0) && (kind == 2 || kind == 4) && S8(NETP + 36) != 0) {
        c->r[6] = GAME;
        call2(c, GAME, BATTLE, f_0882FABC, 0x088D61C8u);
    }
    call2(c, sp + 0x10, *W(sp + 44), f_088D60D0, 0x088D61D8u);
    call2(c, 1, sp + 0x10, f_088E51DC, 0x088D61E4u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x40;
}

/* The link's work every frame: while it is on (0x095236C0 + 16) with something at +4, the incoming
   messages and 0x088E4794. A 0x10 frame. */
SFA_FUNC(088E1AF8, link_every_frame) {
    const gaddr link = 0x095236C0u;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    int32_t on = S8(link + 16);
    *W(c->r[29]) = ra;
    ARG(0) = link; ARG(1) = (uint32_t)on;
    if (on != 0) {
        uint32_t w = *W(link + 4);
        ARG(0) = w;
        if (w != 0) {
            call0(c, f_088E17B0, 0x088E1B28u);
            call0(c, f_088E4794, 0x088E1B30u);
        }
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (fighter): a move's step: at the animation's end 0x0886469C; until its cue (+77 below 0)
   0x088F0BEC. On the cue: without +123 step 7 and 0x088F0C48; with it step 4: with less than 7
   frames left, or not within 96 on the ground of a live opponent whose +296 object's +5 is clear,
   0x088F0B94; else step 8: +260 cleared, 0x20 forward, the pair of objects and the animation
   0x2C. */
SFA_FUNC(088F0A78, move_step_f0a78) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    gaddr other = *W(o + 92);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x088F0A9Cu);
    else if (!(S8(o + 77) < 0)) call1(c, o, f_088F0BEC, 0x088F0AB8u);
    else {
        *B(o + 77) = 0;
        int32_t held = S8(o + 123);
        *B(o + 7) = 7;
        if (held == 0) call1(c, o, f_088F0C48, 0x088F0AD8u);
        else {
            int32_t left = S16(o + 82);
            *B(o + 7) = 4;
            if (left < 7) call1(c, o, f_088F0B94, 0x088F0B00u);
            else {
                int32_t dist = S16(o + 508);
                *B(o + 260) = 0;
                if (dist < 96 && S8(o + 73) == 0 && *B(*W(other + 296) + 5) == 0 && !(S16(other + 112) < 0)) {
                    uint8_t facing = *B(o + 11);
                    *B(o + 7) = 8;
                    int32_t x = S16(o + 18);
                    *H(o + 18) = (uint16_t)(facing != 0 ? x + 0x20 : x - 0x20);
                    call1(c, o, f_088F1E74, 0x088F0B78u);
                    call2(c, o, 0x2C, f_088616EC, 0x088F0B84u);
                } else call1(c, o, f_088F0B94, 0x088F0B44u);
            }
        }
    }
    leave(c, sv);
}

/* (fighter): a held move's first step: ticked until the cue (+77); then step 2 with 0xF frames:
   the cpu holds for the word from 0x08A514EC (by +193) and goes on as 0x089028AC; a player gets
   the strength's shake (0x0886E3CC from the bytes at 0x08A512C8) and goes on as 0x08902698. */
SFA_FUNC(089025F4, held_move_first) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08902688u);
    else {
        *B(o + 7) = 2;
        int32_t cpu = S8(o + 373);
        *H(o + 82) = 0xF;
        if (cpu != 0) {
            *H(o + 82) = (uint16_t)S16(0x08A514ECu + (uint32_t)(S8(o + 193) * 2));
            call1(c, o, f_089028AC, 0x08902648u);
        } else {
            call3(c, o, *B(0x08A512C8u + (uint32_t)(int32_t)S8(o + 194)), 8, f_0886E3CC, 0x08902670u);
            call1(c, o, f_08902698, 0x08902678u);
        }
    }
    leave(c, sv);
}

/* (fighter): an entrance by who the opponent is: against the character 0x1B, a player against the
   cpu (or the other way round) gets 0x0890E804, and two alike one of 0x0890E400 and 0x0890E248 by
   chance; against anyone else the same two by chance the other way round. */
SFA_FUNC(0890E190, entrance_by_opponent_0e190) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    gaddr other = *W(o + 92);
    if (S8(other + 338) == 0x1B) {
        if (S8(o + 373) != S8(other + 373)) call1(c, o, f_0890E804, 0x0890E1D0u);
        else {
            call1(c, 0, f_08821368, 0x0890E1E0u);
            if (c->r[2] & 1) call1(c, o, f_0890E400, 0x0890E1F4u);
            else call1(c, o, f_0890E248, 0x0890E204u);
        }
    } else {
        call1(c, other, f_08821368, 0x0890E214u);
        if (c->r[2] & 1) call1(c, o, f_0890E248, 0x0890E228u);
        else call1(c, o, f_0890E400, 0x0890E238u);
    }
    leave(c, sv);
}

/* (fighter): an entrance by who the opponent is; it is shown. Against the character 0 the cpu
   always, and a player half the time, goes by its choice (0x08924C28). Otherwise: with a cpu on
   either side, against the character 5: step 8 and 0x08924E58; against the character 0x26 the
   three animations in a row; anyone else step 1 and 0x08924B90. A frame keeping s0..s2. */
SFA_FUNC(08924ABC, entrance_by_opponent_24abc) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    gaddr other = *W(o + 92);
    c->r[18] = other; c->r[17] = 1;
    *B(o + 1) = 1;
    int32_t ch = S8(other + 338);
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = o;
    if (ch == 0) {
        int by_choice = S8(o + 373) != 0;
        if (!by_choice) {
            call1(c, 0, f_08821368, 0x08924AFCu);
            by_choice = (int32_t)(c->r[2] & 0xF) < 8;
        }
        if (by_choice) { call1(c, o, f_08924C28, 0x08924B14u); goto out; }
    }
    {
        int cpu = (S8(o + 373) | S8(other + 373)) != 0;
        ch = S8(other + 338);
        c->r[18] = (uint32_t)ch;
        if (cpu && ch == 5) {
            *B(o + 7) = 8;
            call1(c, o, f_08924E58, 0x08924B4Cu);
        } else if (ch == 0x26) call1(c, o, f_0892540C, 0x08924B68u);
        else {
            *B(o + 7) = 1;
            call1(c, o, f_08924B90, 0x08924B78u);
        }
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): an entrance: against the character 7, a player against the cpu (or the other way
   round) takes step 2 and 0x0897AA50; everyone else step 1 and 0x0897AA00. */
SFA_FUNC(0897A990, entrance_by_opponent_7a990) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr other = *W(o + 92);
    int32_t ch = S8(other + 338);
    *W(c->r[29] + 16) = ra;
    c->r[5] = 1; c->r[6] = 7; c->r[7] = (uint32_t)ch;
    if (ch != 7) {
        *B(o + 7) = 1;
        call0(c, f_0897AA00, 0x0897A9F4u);
    } else if (S8(o + 373) != S8(other + 373)) {
        c->r[5] = 2;
        *B(o + 7) = 2;
        call0(c, f_0897AA50, 0x0897A9CCu);
    } else {
        *B(o + 7) = 1;
        call0(c, f_0897AA00, 0x0897A9E0u);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): the first fighter 0x0898155C picks out is attached (0x0898147C): fighters 1 and 2, and
   in a fight with partners (BATTLE + 56) 3 and 4; with none 0x089814CC. A 0x30 frame keeping
   s0..s4. */
SFA_FUNC(08981368, attach_first_match) {
    static const gaddr f[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    static const uint32_t back_ask[4] = { 0x089813B0u, 0x089813D4u, 0x08981408u, 0x0898142Cu };
    static const uint32_t back_take[4] = { 0x089813C4u, 0x089813E8u, 0x0898141Cu, 0x0898143Cu };
    gaddr o = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1];
    *W(sp + 36) = sv[5];
    c->r[20] = FIGHTER_1; c->r[19] = FIGHTER_2; c->r[18] = FIGHTER_3; c->r[17] = FIGHTER_4; c->r[16] = o;
    int i;
    for (i = 0; i < 4; i++) {
        if (i == 2 && S16(BATTLE + 56) == 0) { call1(c, o, f_089814CC, 0x0898145Cu); break; }
        call2(c, o, f[i], f_0898155C, back_ask[i]);
        if (c->r[2] != 0) { call2(c, o, f[i], f_0898147C, back_take[i]); break; }
    }
    if (i == 4) call1(c, o, f_089814CC, 0x0898144Cu);
    for (int k = 0; k < 5; k++) c->r[16 + k] = sv[k];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (fighter): the throw's input for a character: when both 0x0886A264 and 0x0886C380 say 1, the
   character's own start by its choice. Returns whether. A frame keeping s0, s1. */
static void throw_input(Cpu *c, void (*start)(Cpu *), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    call0(c, f_0886A264, back);
    c->r[17] = 1;
    int ok = c->r[2] == 1;
    if (ok) {
        call1(c, o, f_0886C380, back + 0x14);
        ok = c->r[2] == 1;
    }
    if (ok) call1(c, o, start, back + 0x24);
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}
SFA_FUNC(089B577C, throw_input_b577c) { throw_input(c, f_089B75A0, 0x089B5794u); }
SFA_FUNC(089B9BF0, throw_input_b9bf0) { throw_input(c, f_089BDD9C, 0x089B9C08u); }
SFA_FUNC(089C1104, throw_input_c1104) { throw_input(c, f_089C3320, 0x089C111Cu); }
SFA_FUNC(089C6F74, throw_input_c6f74) { throw_input(c, f_089CA0CC, 0x089C6F8Cu); }

/* The meter (+366) pays a cost, down to 0 at most. */
static void pay(gaddr f, int32_t cost) {
    int32_t v = S16(f + 366) - cost;
    *H(f + 366) = (uint16_t)v;
    if ((int16_t)v < 0) *H(f + 366) = 0;
}

/* (fighter): a hit lands on it: the next step. With +595 clear it takes +588, and a set one gives
   the object 0x0886B4D4 returns +356 = 0xC. The character 0xA drops +654 and +656; the character
   0x1C sets +820 and +824. Then 0x088647C4, 0x08864D40 and 0x08864620. A pending cost (+252)
   clears +251, +252 and +502 and is paid from the meter: 0x48, or for the choice 1 the byte from
   0x08A19884 by +808; in a fight with partners both fighters of its side pay. A block (+117 = -1)
   goes to the block's second step. Any other hit: 0x0885C2B4 for the opponent (in a fight with
   partners both of the other side), once per round (BATTLE + 53) the object's +356 = 8, and the
   hit's own start from the table 0x08A197C4 by +117. A frame keeping s0, s1. */
SFA_FUNC(08866598, hit_lands) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    uint8_t step = *B(o + 7);
    int32_t had = S8(o + 595);
    *B(o + 7) = (uint8_t)(step + 1);
    enter2(c, o, sv);
    if (had == 0) {
        int32_t v = S8(o + 588);
        *B(o + 595) = (uint8_t)v;
        if (v != 0) {
            call1(c, o, f_0886B4D4, 0x088665D4u);
            *B(c->r[2] + 356) = 0xC;
        }
    }
    int32_t ch = S8(o + 338);
    if (ch == 0xA && S8(o + 654) != 0) { *B(o + 654) = 0; *B(o + 656) = 0; }
    if (ch == 0x1C) { *W(o + 820) = 1; *W(o + 824) = 1; }
    call1(c, o, f_088647C4, 0x0886661Cu);
    call1(c, o, f_08864D40, 0x08866624u);
    call1(c, o, f_08864620, 0x0886662Cu);
    if (S16(o + 252) != 0) {
        *B(o + 251) = 0;
        *H(o + 252) = 0;
        *B(o + 502) = 0;
        int32_t n = S16(o + 808);
        int32_t cost = 0x48;
        if (n != 0 && S8(o + 386) == 1) cost = S8(0x08A19884u + (uint32_t)n);
        if (S16(BATTLE + 56) == 0) pay(o, cost);
        else if (!(S8(o + 337) & 2)) { pay(FIGHTER_1, cost); pay(FIGHTER_2, cost); }
        else { pay(FIGHTER_3, cost); pay(FIGHTER_4, cost); }
    }
    uint32_t hit = *B(o + 117);
    c->r[17] = 0x092E0000u;
    if (hit == 0xFF) call1(c, o, f_0885C8A8, 0x08866768u);
    else {
        c->r[17] = BATTLE;
        if (S16(BATTLE + 56) == 0) call1(c, *W(o + 92), f_0885C2B4, 0x08866788u);
        else if (!(S8(o + 337) & 2)) {
            call1(c, FIGHTER_3, f_0885C2B4, 0x088667A8u);
            call1(c, FIGHTER_4, f_0885C2B4, 0x088667B4u);
        } else {
            call1(c, FIGHTER_1, f_0885C2B4, 0x088667C4u);
            call1(c, FIGHTER_2, f_0885C2B4, 0x088667D0u);
        }
        if (S8(BATTLE + 53) == 0) {
            *B(BATTLE + 53) = 0xFF;
            call1(c, o, f_0886B4D4, 0x088667E8u);
            *B(c->r[2] + 356) = 8;
        }
        uint32_t fn = *W(0x08A197C4u + ((uint32_t)*B(o + 117) << 2));
        ARG(0) = o; c->r[31] = 0x08866810u;
        rt_call_indirect(c, fn);
    }
    leave2(c, sv);
}
