/* The attract mode's frame with its two steps, a round's start once everyone is ready, and a
   standing fighter's choice of what to do next. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_088A9520(Cpu *c); void f_08857A3C(Cpu *c); void f_0881F6A0(Cpu *c); void f_0887C708(Cpu *c);
void f_08825588(Cpu *c); void f_08820EB4(Cpu *c); void f_0882581C(Cpu *c); void f_0885B640(Cpu *c);
void f_08866824(Cpu *c); void f_08862164(Cpu *c); void f_08863B40(Cpu *c); void f_08863D4C(Cpu *c);
void f_088639C0(Cpu *c); void f_088639CC(Cpu *c); void f_08861E94(Cpu *c); void f_08861EDC(Cpu *c);
void f_08861D5C(Cpu *c); void f_08861D78(Cpu *c); void f_088623BC(Cpu *c); void f_0886163C(Cpu *c);
void f_08862484(Cpu *c); void f_088628EC(Cpu *c); void f_0886469C(Cpu *c); void f_08861848(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* The attract mode's frame: its script (0x088A9520), then the picture's step and the sound's
   (the words at 0x08B0274C and 0x08B02750, when not negative) from the table 0x08A2C248. A 0x10
   frame keeping s0. */
SFA_FUNC(088A94AC, attract_frame) {
    const gaddr st = 0x08B0274Cu;
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    call0(c, f_088A9520, 0x088A94C0u);
    int32_t picture = S16(st);
    c->r[16] = st;
    if (picture >= 0) {
        uint32_t fn = *W(0x08A2C248u + ((uint32_t)picture << 2));
        ARG(0) = fn; ARG(1) = 0x08A2C248u; c->r[31] = 0x088A94ECu;
        rt_call_indirect(c, fn);
    }
    int32_t sound = S16(st + 4);
    c->r[16] = (uint32_t)sound;
    if (sound >= 0) {
        uint32_t fn = *W(0x08A2C248u + ((uint32_t)sound << 2));
        ARG(0) = fn; ARG(1) = 0x08A2C248u; c->r[31] = 0x088A9510u;
        rt_call_indirect(c, fn);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (x, y): a round starts once y's +44 is 6 and the byte at 0x095176F2 has reached 2: x's +6 moves
   on with +8 and +10 cleared, y's +44 and +14 are cleared and +45 set, the three bytes at
   0x092E8800, 0x092E88A0 and 0x092E8940 are set, the four fighters' +372 and +415 cleared,
   0x08857A3C and 0x0881F6A0 run, and the sound thread restarts for a first round (y + 2), or in
   the fights through lists (y + 155) unless y's +63 is set. Where partners fight (the mode 0xF,
   the mode 7 with y's +79 at 5, the mode 0x10 with y's +159 at 3) they are paired and BATTLE's
   +154 is 6 or 7 by the side (x + 53); in the mode 0x13 they are paired and +154 is the cpu's
   team. 0x0882581C ends every call. A frame keeping s0..s2. */
SFA_FUNC(0882565C, round_start_when_ready) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = y;
    *W(sp + 20) = s1; c->r[17] = x;
    int32_t state = S8(y + 44);
    *W(sp + 24) = s2;
    c->r[18] = 6;
    *W(sp + 28) = ra;
    if (state == 6 && !(S8(0x095176F2u) < 2)) {
        *H(x + 10) = 0;
        *H(x + 6) = (uint16_t)(*H(x + 6) + 1);
        *H(x + 8) = 0;
        *B(y + 44) = 0;
        *B(y + 14) = 0;
        *B(y + 45) = 1;
        *B(0x092E8800u) = 1;
        *B(0x092E88A0u) = 1;
        *B(0x092E8940u) = 1;
        *B(FIGHTER_4 + 372) = 0; *B(FIGHTER_3 + 372) = 0; *B(FIGHTER_2 + 372) = 0; *B(FIGHTER_1 + 372) = 0;
        *B(FIGHTER_4 + 415) = 0; *B(FIGHTER_3 + 415) = 0; *B(FIGHTER_2 + 415) = 0; *B(FIGHTER_1 + 415) = 0;
        ARG(0) = FIGHTER_4; ARG(1) = FIGHTER_3; ARG(2) = FIGHTER_2; ARG(3) = FIGHTER_1;
        call0(c, f_08857A3C, 0x0882571Cu);
        call0(c, f_0881F6A0, 0x08825724u);
        if (S16(y + 2) == 0) call0(c, f_0887C708, 0x08825738u);
        else if (S8(y + 155) != 0 && S8(y + 63) == 0) call0(c, f_0887C708, 0x08825760u);
        int32_t mode = S8(x + 114);
        if (mode == 0xF || (mode == 7 && S8(y + 79) == 5) || (mode == 0x10 && S8(y + 159) == 3)) {
            call0(c, f_08825588, 0x088257ACu);
            *B(BATTLE + 154) = (S8(x + 53) & 1) ? 6 : 7;
        } else if (mode == 0x13) {
            call0(c, f_08825588, 0x088257E4u);
            call0(c, f_08820EB4, 0x088257ECu);
            *B(BATTLE + 154) = (uint8_t)c->r[2];
        }
    }
    call2(c, x, y, f_0882581C, 0x08825804u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a standing fighter's next move. The cpu's (+373) goes to 0x0885B640. A player's is
   the first of these that applies: the partner standing by is called in; then five checks each
   with its own start (0x08863B40, 0x088639C0, 0x08861E94, 0x08861D5C, and 0x088623BC, which moves
   +7 on and starts the move 0xD past its +77); 0x08862484 with 0x088628EC; with the top bit of
   +75 set 0x0886469C; and otherwise it turns to face (+697 to +11) and 0x08861848 runs. */
SFA_FUNC(0885B4E4, standing_next_move) {
    gaddr f = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    int32_t cpu = S8(f + 373);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    static const struct { void (*check)(Cpu *); uint32_t back; void (*start)(Cpu *); uint32_t back2; } t[4] = {
        { f_08863B40, 0x0885B528u, f_08863D4C, 0x0885B538u },
        { f_088639C0, 0x0885B548u, f_088639CC, 0x0885B558u },
        { f_08861E94, 0x0885B568u, f_08861EDC, 0x0885B578u },
        { f_08861D5C, 0x0885B588u, f_08861D78, 0x0885B598u },
    };
    if (cpu != 0) call1(c, f, f_0885B640, 0x0885B630u);
    else {
        int done = 0;
        call2(c, f, 0, f_08866824, 0x0885B508u);
        if (c->r[2] != 0) { call1(c, f, f_08862164, 0x0885B518u); done = 1; }
        for (int i = 0; i < 4 && !done; i++) {
            call1(c, f, t[i].check, t[i].back);
            if (c->r[2] != 0) { call1(c, f, t[i].start, t[i].back2); done = 1; }
        }
        if (!done) {
            call1(c, f, f_088623BC, 0x0885B5A8u);
            if (c->r[2] != 0) {
                *B(f + 7) = (uint8_t)(*B(f + 7) + 1);
                call2(c, f, ((uint32_t)(int32_t)S8(f + 77) + 0xD) & 0xFFFF, f_0886163C, 0x0885B5D0u);
                done = 1;
            }
        }
        if (!done) {
            call1(c, f, f_08862484, 0x0885B5E0u);
            if (c->r[2] != 0) call1(c, f, f_088628EC, 0x0885B5F0u);
            else if (S8(f + 75) & 0x80) call1(c, f, f_0886469C, 0x0885B60Cu);
            else {
                *B(f + 11) = (uint8_t)S8(f + 697);
                call1(c, f, f_08861848, 0x0885B620u);
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
