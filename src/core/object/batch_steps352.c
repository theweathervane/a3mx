/* Two starts of a fighter's walk with the choice that follows, the walk itself with its own
   choice, and the attract mode's whole frame. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886163C(Cpu *c); void f_0885A2C4(Cpu *c); void f_0885A500(Cpu *c); void f_088A94AC(Cpu *c);
void f_088A9748(Cpu *c); void f_08887AFC(Cpu *c); void f_08837690(Cpu *c); void f_088379B8(Cpu *c);
void f_0883771C(Cpu *c); void f_0880B6CC(Cpu *c); void f_0881EE84(Cpu *c); void f_088AC4D8(Cpu *c);
void f_0881A040(Cpu *c); void f_0885A7B8(Cpu *c); void f_0885A728(Cpu *c); void f_088629CC(Cpu *c);
void f_088632C4(Cpu *c); void f_08863504(Cpu *c); void f_08862B84(Cpu *c); void f_08862C48(Cpu *c);
void f_08864284(Cpu *c); void f_08861848(Cpu *c); void f_08866824(Cpu *c); void f_08862164(Cpu *c);
void f_08863B40(Cpu *c); void f_08863D4C(Cpu *c); void f_088639C0(Cpu *c); void f_088639CC(Cpu *c);
void f_08861E94(Cpu *c); void f_08861EDC(Cpu *c); void f_08862484(Cpu *c); void f_088628EC(Cpu *c);
void f_088623BC(Cpu *c); void f_088623E4(Cpu *c); void f_088622C0(Cpu *c); void f_08864650(Cpu *c);
void f_088622CC(Cpu *c); void f_088622E8(Cpu *c); void f_08861E30(Cpu *c);

typedef struct { void (*check)(Cpu *); void (*start)(Cpu *); uint32_t back; } Try;

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static int try_all(Cpu *c, gaddr f, const Try *t, int n) {
    for (int i = 0; i < n; i++) {
        call1(c, f, t[i].check, t[i].back);
        if (c->r[2] != 0) { call1(c, f, t[i].start, t[i].back + 0x10); return 1; }
    }
    return 0;
}
static void enter(Cpu *c, gaddr f, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = f;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): its +7 moves on, the cpu's starts the move 7, and the choice 0x0885A2C4 follows. */
SFA_FUNC(0885A27C, step_on_then_choice_c) {
    gaddr f = ARG(0);
    uint32_t sv[2];
    *B(f + 7) = (uint8_t)(*B(f + 7) + 1);
    int32_t cpu = S8(f + 373);
    enter(c, f, sv);
    if (cpu != 0) call2(c, f, 7, f_0886163C, 0x0885A2ACu);
    call1(c, f, f_0885A2C4, 0x0885A2B4u);
    leave(c, sv);
}

/* (fighter): its +7 moves on, the cpu's starts the move 4, and the choice 0x0885A500 follows. */
SFA_FUNC(0885A4B8, step_on_then_choice_d) {
    gaddr f = ARG(0);
    uint32_t sv[2];
    *B(f + 7) = (uint8_t)(*B(f + 7) + 1);
    int32_t cpu = S8(f + 373);
    enter(c, f, sv);
    if (cpu != 0) call2(c, f, 4, f_0886163C, 0x0885A4E8u);
    call1(c, f, f_0885A500, 0x0885A4F0u);
    leave(c, sv);
}

/* The attract mode's whole frame: GAME's +14 counts on, the script and its steps, 0x088A9748
   unless the picture's step is 8, the screen's parts (0x08887AFC, 0x08837690, 0x088379B8,
   0x0883771C), 0x0880B6CC, 0x0881EE84, 0x088AC4D8 with the state at 0x08B0274C, and
   0x0881A040. */
SFA_FUNC(088A93B0, attract_whole_frame) {
    uint32_t sv[2];
    uint32_t t = (uint32_t)(int32_t)S16(GAME + 14) + 1;
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    *H(GAME + 14) = (uint16_t)t;
    call2(c, GAME, t, f_088A94AC, 0x088A93D4u);
    c->r[16] = 0x08B00000u;
    if (S16(0x08B0274Cu) != 8) call0(c, f_088A9748, 0x088A93F0u);
    call0(c, f_08887AFC, 0x088A93F8u);
    call0(c, f_08837690, 0x088A9400u);
    call0(c, f_088379B8, 0x088A9408u);
    call0(c, f_0883771C, 0x088A9410u);
    call0(c, f_0880B6CC, 0x088A9418u);
    call0(c, f_0881EE84, 0x088A9420u);
    call1(c, 0x08B0274Cu, f_088AC4D8, 0x088A9428u);
    call0(c, f_0881A040, 0x088A9430u);
    leave(c, sv);
}

/* (fighter): a walk starts: it turns to face (+697 to +11), the move 2 (3 backwards, by +78)
   starts, and its speed (+96) is the character's (+338) forward or backward one from the table
   at +704, to the left when facing that way; then the walk's choice. */
SFA_FUNC(0885A728, walk_start) {
    gaddr f = ARG(0);
    uint32_t sv[2];
    enter(c, f, sv);
    int32_t back = S8(f + 78);
    c->r[6] = (uint32_t)back;
    *B(f + 11) = (uint8_t)S8(f + 697);
    call2(c, f, back != 0 ? 3 : 2, f_0886163C, 0x0885A75Cu);
    gaddr speeds = *W(f + 704) + ((uint32_t)(int32_t)S8(f + 338) << 2);
    uint32_t v = (uint32_t)(int32_t)S16(speeds + (S8(f + 78) != 0 ? 2 : 0)) << 8;
    if (*B(f + 11) != 0) v = 0 - v;
    *W(f + 96) = v;
    call1(c, f, f_0885A7B8, 0x0885A7A8u);
    leave(c, sv);
}

/* (fighter): the walk's choice. While BATTLE's +13 is set 0x088629CC. Else three checks (the
   third, 0x08864284, stops the walk). A player's may call the partner in and has five more
   checks; without 0x088622C0 it goes to 0x08864650; else 0x088622CC with its start, or
   0x08861E30 and the walk stopped. The cpu's starts the walk again with 0x088622CC or when its
   wanted direction (+497) is not the one it walks in (+78), and otherwise takes a step (+96 added
   to +16) and goes on. */
SFA_FUNC(0885A7B8, walk_choice) {
    gaddr f = ARG(0);
    static const Try first[3] = {
        { f_088632C4, f_08863504, 0x0885A7F0u }, { f_08862B84, f_08862C48, 0x0885A810u },
        { f_08864284, f_08861848, 0x0885A830u },
    };
    static const Try rest[5] = {
        { f_08863B40, f_08863D4C, 0x0885A878u }, { f_088639C0, f_088639CC, 0x0885A898u },
        { f_08861E94, f_08861EDC, 0x0885A8B8u }, { f_08862484, f_088628EC, 0x0885A8D8u },
        { f_088623BC, f_088623E4, 0x0885A8F8u },
    };
    static const Try last[1] = { { f_088622CC, f_088622E8, 0x0885A938u } };
    uint32_t sv[2];
    int32_t held = S8(BATTLE + 13);
    enter(c, f, sv);
    if (held != 0) call1(c, f, f_088629CC, 0x0885A7E0u);
    else if (!try_all(c, f, first, 3)) {
        if (S8(f + 373) == 0) {
            int done = 0;
            call2(c, f, 0, f_08866824, 0x0885A858u);
            if (c->r[2] != 0) { call1(c, f, f_08862164, 0x0885A868u); done = 1; }
            if (!done && !try_all(c, f, rest, 5)) {
                call1(c, f, f_088622C0, 0x0885A918u);
                if (c->r[2] == 0) call1(c, f, f_08864650, 0x0885A928u);
                else if (!try_all(c, f, last, 1)) {
                    call1(c, f, f_08861E30, 0x0885A958u);
                    call1(c, f, f_08861848, 0x0885A960u);
                }
            }
        } else {
            call1(c, f, f_088622CC, 0x0885A970u);
            if (c->r[2] != 0) call1(c, f, f_0885A728, 0x0885A980u);
            else if (S8(f + 78) != S8(f + 497)) {
                *B(f + 78) = (uint8_t)S8(f + 497);
                call1(c, f, f_0885A728, 0x0885A9A0u);
            } else {
                *W(f + 16) = *W(f + 16) + *W(f + 96);
                call1(c, f, f_08861848, 0x0885A9BCu);
            }
        }
    }
    leave(c, sv);
}
