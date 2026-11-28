/* Four more of a fighter's choices of what to do next: each asks a row of checks in turn and
   starts what the first one that says yes belongs to. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088629CC(Cpu *c); void f_088632C4(Cpu *c); void f_08863504(Cpu *c); void f_08862B84(Cpu *c);
void f_08862C48(Cpu *c); void f_08864284(Cpu *c); void f_088622CC(Cpu *c); void f_088622E8(Cpu *c);
void f_08861848(Cpu *c); void f_08866824(Cpu *c); void f_08862164(Cpu *c); void f_08863B40(Cpu *c);
void f_08863D4C(Cpu *c); void f_088639C0(Cpu *c); void f_088639CC(Cpu *c); void f_088623BC(Cpu *c);
void f_088623E4(Cpu *c); void f_08861E94(Cpu *c); void f_08861EDC(Cpu *c); void f_08862484(Cpu *c);
void f_088628EC(Cpu *c); void f_08861D5C(Cpu *c); void f_08861D78(Cpu *c); void f_0885A1DC(Cpu *c);
void f_0886244C(Cpu *c); void f_0885A458(Cpu *c); void f_0886163C(Cpu *c); void f_0885A66C(Cpu *c);

typedef struct { void (*check)(Cpu *); void (*start)(Cpu *); uint32_t back; } Try;

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* The checks of t in turn; the first that says yes has its start run. 1 when one did. */
static int try_all(Cpu *c, gaddr f, const Try *t, int n) {
    for (int i = 0; i < n; i++) {
        call1(c, f, t[i].check, t[i].back);
        if (c->r[2] != 0) { call1(c, f, t[i].start, t[i].back + 0x10); return 1; }
    }
    return 0;
}
/* The partner standing by is called in. 1 when it was. */
static int try_partner(Cpu *c, gaddr f, uint32_t back) {
    call2(c, f, 0, f_08866824, back);
    if (c->r[2] == 0) return 0;
    call1(c, f, f_08862164, back + 0x10);
    return 1;
}
static void enter(Cpu *c, gaddr f, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = f;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): with its +696 cleared. While BATTLE's +13 is set 0x088629CC. Else two checks; then
   0x08864284 or BATTLE's +14 keeps it as it is (0x08861848); one more check; the cpu's stays as
   it is; a player's may call the partner in and has six more checks before it stays as it is. A
   frame keeping s0, s1. */
SFA_FUNC(08859E80, next_move_a) {
    gaddr f = ARG(0);
    static const Try first[2] = {
        { f_088632C4, f_08863504, 0x08859EC0u }, { f_08862B84, f_08862C48, 0x08859EE0u },
    };
    static const Try third[1] = { { f_088622CC, f_088622E8, 0x08859F18u } };
    static const Try rest[6] = {
        { f_08863B40, f_08863D4C, 0x08859F70u }, { f_088639C0, f_088639CC, 0x08859F90u },
        { f_088623BC, f_088623E4, 0x08859FB0u }, { f_08861E94, f_08861EDC, 0x08859FD0u },
        { f_08862484, f_088628EC, 0x08859FF0u }, { f_08861D5C, f_08861D78, 0x0885A010u },
    };
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    *B(f + 696) = 0;
    c->r[17] = BATTLE;
    int32_t held = S8(BATTLE + 13);
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = f;
    if (held != 0) call1(c, f, f_088629CC, 0x08859EB0u);
    else if (!try_all(c, f, first, 2)) {
        call1(c, f, f_08864284, 0x08859F00u);
        if ((c->r[2] | (uint32_t)(int32_t)S8(BATTLE + 14)) != 0) call1(c, f, f_08861848, 0x0885A040u);
        else if (!try_all(c, f, third, 1)) {
            if (S8(f + 373) != 0) call1(c, f, f_08861848, 0x08859F40u);
            else if (!try_partner(c, f, 0x08859F50u) && !try_all(c, f, rest, 6))
                call1(c, f, f_08861848, 0x0885A030u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with its +696 set. While BATTLE's +13 is set 0x088629CC. Else four checks (the third
   with 0x0885A1DC); the cpu's goes to 0x0885A1DC; a player's may call the partner in and has two
   more checks; then without 0x088623BC it goes to 0x0886244C, and with it to 0x088628EC when
   0x08862484 agrees and to 0x0885A1DC otherwise. */
SFA_FUNC(0885A054, next_move_b) {
    gaddr f = ARG(0);
    static const Try first[4] = {
        { f_088632C4, f_08863504, 0x0885A094u }, { f_08862B84, f_08862C48, 0x0885A0B4u },
        { f_08864284, f_0885A1DC, 0x0885A0D4u }, { f_088622CC, f_088622E8, 0x0885A0F4u },
    };
    static const Try rest[2] = {
        { f_08863B40, f_08863D4C, 0x0885A13Cu }, { f_088639C0, f_088639CC, 0x0885A15Cu },
    };
    static const Try last[1] = { { f_08862484, f_088628EC, 0x0885A19Cu } };
    uint32_t sv[2];
    *B(f + 696) = 1;
    int32_t held = S8(BATTLE + 13);
    enter(c, f, sv);
    if (held != 0) call1(c, f, f_088629CC, 0x0885A084u);
    else if (!try_all(c, f, first, 4)) {
        if (S8(f + 373) != 0) call1(c, f, f_0885A1DC, 0x0885A1CCu);
        else if (!try_partner(c, f, 0x0885A11Cu) && !try_all(c, f, rest, 2)) {
            call1(c, f, f_088623BC, 0x0885A17Cu);
            if (c->r[2] == 0) call1(c, f, f_0886244C, 0x0885A18Cu);
            else if (!try_all(c, f, last, 1)) call1(c, f, f_0885A1DC, 0x0885A1BCu);
        }
    }
    leave(c, sv);
}

/* (fighter): while BATTLE's +13 is set 0x088629CC. Else two checks; the cpu's goes to
   0x0885A458; a player's may call the partner in and has six more checks (0x088623BC with
   0x0885A458); when none says yes its +7 moves on and the move 4 past its +77 starts. */
SFA_FUNC(0885A2C4, next_move_c) {
    gaddr f = ARG(0);
    static const Try first[2] = {
        { f_088632C4, f_08863504, 0x0885A2FCu }, { f_08862B84, f_08862C48, 0x0885A31Cu },
    };
    static const Try rest[6] = {
        { f_08863B40, f_08863D4C, 0x0885A364u }, { f_088639C0, f_088639CC, 0x0885A384u },
        { f_08861E94, f_08861EDC, 0x0885A3A4u }, { f_08862484, f_088628EC, 0x0885A3C4u },
        { f_088623BC, f_0885A458, 0x0885A3E4u }, { f_08861D5C, f_08861D78, 0x0885A404u },
    };
    uint32_t sv[2];
    int32_t held = S8(BATTLE + 13);
    enter(c, f, sv);
    if (held != 0) call1(c, f, f_088629CC, 0x0885A2ECu);
    else if (!try_all(c, f, first, 2)) {
        if (S8(f + 373) != 0) call1(c, f, f_0885A458, 0x0885A448u);
        else if (!try_partner(c, f, 0x0885A344u) && !try_all(c, f, rest, 6)) {
            *B(f + 7) = (uint8_t)(*B(f + 7) + 1);
            call2(c, f, ((uint32_t)(int32_t)S8(f + 77) + 4) & 0xFFFF, f_0886163C, 0x0885A438u);
        }
    }
    leave(c, sv);
}

/* (fighter): while BATTLE's +13 is set 0x088629CC. Else two checks; the cpu's goes to
   0x0885A66C; a player's may call the partner in and has four more checks; then with 0x088623BC
   its +7 is cleared and the move 7 past its +77 starts, and without it 0x0885A66C. */
SFA_FUNC(0885A500, next_move_d) {
    gaddr f = ARG(0);
    static const Try first[2] = {
        { f_088632C4, f_08863504, 0x0885A538u }, { f_08862B84, f_08862C48, 0x0885A558u },
    };
    static const Try rest[4] = {
        { f_08863B40, f_08863D4C, 0x0885A5A0u }, { f_088639C0, f_088639CC, 0x0885A5C0u },
        { f_08861E94, f_08861EDC, 0x0885A5E0u }, { f_08862484, f_088628EC, 0x0885A600u },
    };
    uint32_t sv[2];
    int32_t held = S8(BATTLE + 13);
    enter(c, f, sv);
    if (held != 0) call1(c, f, f_088629CC, 0x0885A528u);
    else if (!try_all(c, f, first, 2)) {
        if (S8(f + 373) != 0) call1(c, f, f_0885A66C, 0x0885A65Cu);
        else if (!try_partner(c, f, 0x0885A580u) && !try_all(c, f, rest, 4)) {
            call1(c, f, f_088623BC, 0x0885A620u);
            if (c->r[2] != 0) {
                int32_t base = S8(f + 77);
                *B(f + 7) = 0;
                call2(c, f, ((uint32_t)base + 7) & 0xFFFF, f_0886163C, 0x0885A64Cu);
            } else call1(c, f, f_0885A66C, 0x0885A630u);
        }
    }
    leave(c, sv);
}
