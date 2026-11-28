/* Seven callers of the hand-written pieces: three of the save screen's steps around its question,
   a frame's end while the fight waits, a fighter's turn at the continue screen and after it, and
   whether a new press is one of two buttons. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08818BA4(Cpu *c); void f_089E3EA8(Cpu *c); void f_088168E0(Cpu *c); void f_08816A78(Cpu *c);
void f_088054F0(Cpu *c); void f_0880509C(Cpu *c); void f_088347B4(Cpu *c); void f_08834604(Cpu *c);
void f_0882E264(Cpu *c); void f_08833DDC(Cpu *c); void f_08834298(Cpu *c); void f_08834150(Cpu *c);
void f_08867794(Cpu *c);

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
static void enter3(Cpu *c, uint32_t sv[4]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[18]; sv[3] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    *W(c->r[29] + 28) = sv[3];
}
static void leave3(Cpu *c, const uint32_t sv[4]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = sv[3]; c->r[29] += 0x20;
}

/* (p): the save screen asks before it saves or loads. Without a memory stick (p + 7) it goes to
   the step 6 with the text 0x13. The question (0x08818BA4) answered no gives the step 8 with the
   text 0x49 (0x4A for a load) and -1 at +9; answered yes, once the word at 0x08AD7CD4 is set, the
   step 3 and the save (text 0xD) or the load (text 0x10) of the kind at +10 in the slot at +3. */
SFA_FUNC(08817A4C, save_screen_ask) {
    gaddr p = ARG(0);
    uint32_t sv[2];
    int32_t stick = S8(p + 7);
    enter(c, p, sv);
    if (stick == 0) {
        *B(p + 2) = 6;
        *B(p + 4) = 0x13;
        *B(p + 9) = 0;
    } else {
        call1(c, p, f_08818BA4, 0x08817A6Cu);
        int32_t answer = (int32_t)c->r[2];
        if (answer == -1) {
            *B(p + 4) = S8(p + 8) == 0 ? 0x4A : 0x49;
            *B(p + 2) = 8;
            *B(p + 9) = 0xFF;
        } else if (answer == 1) {
            ARG(0) = 1; ARG(1) = 1;
            c->r[31] = 0x08817AECu; f_089E3EA8(c);
            if (c->r[2] != 0) {
                int32_t saving = S8(p + 8);
                *B(p + 2) = 3;
                uint32_t kind = (uint32_t)(int32_t)S8(p + 10), slot = (uint32_t)(int32_t)S8(p + 3);
                c->r[6] = slot;
                if (saving != 0) {
                    *B(p + 4) = 0xD;
                    call2(c, kind, slot, f_088168E0, 0x08817B20u);
                } else {
                    *B(p + 4) = 0x10;
                    call2(c, kind, slot, f_08816A78, 0x08817B40u);
                }
            }
        }
    }
    leave(c, sv);
}

/* (p): the question's answer moves the screen: yes to the step 5, no to the step 7 with -1 at
   +11. */
SFA_FUNC(08817F94, save_screen_answer_5_7) {
    gaddr p = ARG(0);
    uint32_t sv[2];
    enter(c, p, sv);
    c->r[31] = 0x08817FA8u; f_08818BA4(c);
    int32_t answer = (int32_t)c->r[2];
    if (answer == 1) *B(p + 2) = 5;
    else if (answer == -1) { *B(p + 2) = 7; *B(p + 11) = 0xFF; }
    leave(c, sv);
}

/* (p): the same to the step 0 for yes and the step 1 for no. */
SFA_FUNC(088183CC, save_screen_answer_0_1) {
    gaddr p = ARG(0);
    uint32_t sv[2];
    enter(c, p, sv);
    c->r[31] = 0x088183E0u; f_08818BA4(c);
    int32_t answer = (int32_t)c->r[2];
    if (answer == 1) *B(p + 2) = 0;
    else if (answer == -1) { *B(p + 2) = 1; *B(p + 11) = 0xFF; }
    leave(c, sv);
}

/* (x, y): the frame's end: a plain one (0x088054F0 with 1) while x's +36 is set and y's +23 is
   not, and 0x0880509C otherwise. */
SFA_FUNC(0882362C, frame_end_by_wait) {
    gaddr y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t wait = S8(ARG(0) + 36);
    ARG(0) = (uint32_t)wait;
    *W(c->r[29] + 16) = ra;
    if (wait != 0 && S8(y + 23) == 0) call1(c, 1, f_088054F0, 0x08823654u);
    else {
        if (wait != 0) ARG(0) = (uint32_t)(int32_t)S8(y + 23);
        c->r[31] = 0x08823668u; f_0880509C(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y, fighter): a fighter waiting to come in. While anything holds the screen (y's +4, +6 and
   +70, x's +40 and +43) its +412 is cleared. The first free frame marks +341 and 0x088347B4 (1)
   runs. Afterwards 0x088347B4 with 1 (with x's +32 bit 0x20 set: the fighter's +360, 0 when that
   is 1), and when 0x08834604 says it came in: its +340 moves on, its counters are cleared, +448
   takes its side (+337), +449 is 2, the screen is black and 0x08833DDC runs. A frame keeping
   s0..s2. */
SFA_FUNC(08833C8C, fighter_waiting_in) {
    gaddr x = ARG(0), y = ARG(1), f = ARG(2);
    uint32_t sv[4];
    int32_t busy = S8(y + 4) | S8(y + 6) | S8(x + 40) | S8(y + 70) | S8(x + 43);
    enter3(c, sv);
    c->r[18] = x; c->r[17] = y; c->r[16] = f;
    if (busy != 0) *B(f + 412) = 0;
    else if (S8(f + 341) == 0) {
        *B(f + 341) = 1;
        call2(c, f, 1, f_088347B4, 0x08833CF8u);
    } else {
        uint32_t how = 1;
        if (*B(x + 32) & 0x20) {
            int32_t v = S8(f + 360);
            c->r[6] = 1;
            how = v == 1 ? 0 : ((uint32_t)v & 0xFFFF);
        }
        call2(c, f, how, f_088347B4, 0x08833D3Cu);
        call3(c, x, y, f, f_08834604, 0x08833D4Cu);
        if (c->r[2] != 0) {
            *B(f + 340) = (uint8_t)(S8(f + 340) + 1);
            *B(f + 450) = 0;
            *B(f + 429) = 0;
            *H(f + 404) = 0;
            *W(f + 400) = 0;
            *B(f + 361) = 0;
            *B(f + 411) = 0;
            *B(f + 407) = 0;
            *W(f + 352) = 0;
            *B(f + 341) = 0;
            *B(f + 448) = (uint8_t)S8(f + 337);
            *B(f + 449) = 2;
            call1(c, 0, f_0882E264, 0x08833DB4u);
            call3(c, x, y, f, f_08833DDC, 0x08833DC4u);
        }
    }
    leave3(c, sv);
}

/* (x, y, fighter): 0x08834298, then by 0x08834604: 0 leaves it to 0x08834150; otherwise the
   fighter comes back: its side's bit of x's +42 is cleared, +340 is 2, +341 cleared, +407 and
   +350 set, its count of comebacks (+361) moves on, and unless that stood at 99 its +352 moves on
   and wraps at 100 (its partner's too in the mode 0xF); then 0x08833DDC. A frame keeping
   s0..s2. */
SFA_FUNC(0883402C, fighter_comes_back) {
    gaddr x = ARG(0), y = ARG(1), f = ARG(2);
    uint32_t sv[4];
    enter3(c, sv);
    c->r[16] = f; c->r[17] = y; c->r[18] = x;
    c->r[31] = 0x08834050u; f_08834298(c);
    call3(c, x, y, f, f_08834604, 0x08834060u);
    if (c->r[2] == 0) call3(c, x, y, f, f_08834150, 0x08834074u);
    else {
        uint32_t bit = 1u << ((uint32_t)S8(f + 337) & 31);
        *B(x + 42) = (uint8_t)((uint32_t)(int32_t)S8(x + 42) & (0xFF - bit));
        *B(f + 340) = 2;
        *B(f + 341) = 0;
        *B(f + 407) = 1;
        *H(f + 350) = 1;
        int32_t times = S8(f + 361);
        *B(f + 361) = (uint8_t)(times + 1);
        if (times != 0x63) {
            uint32_t w = *W(f + 352) + 1;
            int32_t mode = S8(GAME + 114);
            DIV(c, w, 0x64);
            *W(f + 352) = w;
            if (mode == 0xF) {
                gaddr partner = *W(f + 1168);
                *W(f + 352) = c->hi;
                uint32_t w2 = *W(partner + 352) + 1;
                DIV(c, w2, 0x64);
                *W(partner + 352) = w2;
                *W(partner + 352) = c->hi;
            } else *W(f + 352) = c->hi;
        }
        call3(c, x, y, f, f_08833DDC, 0x08834138u);
    }
    leave3(c, sv);
}

/* (fighter): 1 when one of its new presses (0x08867794 with 0) is the button 0x36 in its table of
   eight at +1150, 2 for the button 0x37, and 0 for neither; the first of the eight that is
   pressed and either decides. */
SFA_FUNC(088679BC, new_press_36_or_37) {
    gaddr f = ARG(0);
    uint32_t sv[2];
    enter(c, f, sv);
    call2(c, f, 0, f_08867794, 0x088679D4u);
    uint32_t pressed = c->r[2];
    uint32_t found = 0;
    for (uint32_t i = 0; i < 8; i++) {
        if (!(pressed & (1u << i))) continue;
        int32_t button = S16(f + 2 * i + 1150);
        if (button == 0x36) { found = 1; break; }
        if (button == 0x37) { found = 2; break; }
    }
    c->r[2] = found;
    leave(c, sv);
}

#define BATTLE 0x092E1228u

void f_088E3604(Cpu *c); void f_08822600(Cpu *c); void f_088D5E44(Cpu *c); void f_088D63E8(Cpu *c);
void f_0882E380(Cpu *c); void f_08823BEC(Cpu *c); void f_08821FC8(Cpu *c); void f_08837690(Cpu *c);
void f_0883771C(Cpu *c); void f_0880B6CC(Cpu *c); void f_08819144(Cpu *c); void f_0883B800(Cpu *c);
void f_08821F9C(Cpu *c); void f_0880ABD4(Cpu *c); void f_08813770(Cpu *c); void f_08814384(Cpu *c);
void f_089DEBE8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }

/* (x, y): a screen's wait of 0x20 frames once 0x088E3604 agrees; when it is over x's +2 moves on
   with its counters cleared and +134 set, a player's fight gets 0x088D63E8, the fight is prepared
   (0x0882E380), x's +41 and +157 are cleared, and 0x08823BEC and 0x08821FC8 run. The screen's two
   parts are drawn while it waits; 0x0880B6CC and 0x08819144 end every frame. A frame keeping s0,
   s1. */
SFA_FUNC(088944AC, screen_wait_then_fight) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = y;
    *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[17] = x;
    call0(c, f_088E3604, 0x088944C8u);
    if (c->r[2] != 0) {
        call0(c, f_08822600, 0x088944D8u);
        if (*H(x + 6) == 0) {
            *H(x + 6) = 1;
            *H(x + 18) = 0x20;
        }
        int32_t t = S16(x + 18) - 1;
        *H(x + 18) = (uint16_t)t;
        if ((int16_t)t == 0) {
            *H(x + 2) = (uint16_t)(*H(x + 2) + 1);
            *B(x + 56) = 0;
            *H(x + 22) = 0; *H(x + 20) = 0; *H(x + 18) = 0; *H(x + 10) = 0;
            *H(x + 8) = 0; *H(x + 6) = 0; *H(x + 4) = 0;
            *B(x + 134) = 1;
            ARG(0) = 0; ARG(1) = 1;
            call0(c, f_088D5E44, 0x0889454Cu);
            if (c->r[2] != 0) call1(c, y, f_088D63E8, 0x0889455Cu);
            call2(c, x, BATTLE, f_0882E380, 0x0889456Cu);
            *B(x + 41) = 0;
            call2(c, GAME, y, f_08823BEC, 0x08894580u);
            call0(c, f_08821FC8, 0x08894588u);
            *B(x + 157) = 0;
        }
        call0(c, f_08837690, 0x08894594u);
        call0(c, f_0883771C, 0x0889459Cu);
    }
    call0(c, f_0880B6CC, 0x088945A4u);
    call0(c, f_08819144, 0x088945ACu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): unless x's +93 is set, the byte at 0x092E891B is cleared and x's step (+4) is chosen by
   the mode: where a challenger may come in (0x0883B800: the mode 7 in the kinds 1, 2, 3 and 6 of
   y's +79, the modes 0x11 and 0x12, the mode 9 with x's +191 set) the step 9 when one does; else
   the next step in the modes 7 and 0x12, and the step 8 in the others. */
SFA_FUNC(088D1F9C, step_by_challenger) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[2];
    int32_t held = S8(x + 93);
    enter(c, x, sv);
    if (held == 0) {
        *B(0x092E88A0u + 123) = 0;
        int32_t mode = S8(x + 114);
        int ask = 0, next = 0;
        uint32_t back = 0;
        if (mode == 7) {
            int32_t kind = S8(y + 79);
            next = 1;
            if (kind == 1 || kind == 2 || kind == 3 || kind == 6) { ask = 1; back = 0x088D2008u; }
        } else if (mode == 0x12) { ask = 1; next = 1; back = 0x088D2048u; }
        else if (mode == 0x11) { ask = 1; back = 0x088D2088u; }
        else if (mode == 9 && S8(x + 191) != 0) { ask = 1; back = 0x088D20CCu; }
        int came = 0;
        if (ask) {
            call2(c, x, y, f_0883B800, back);
            came = c->r[2] != 0;
        }
        if (came) *H(x + 4) = 9;
        else if (next) *H(x + 4) = (uint16_t)(*H(x + 4) + 1);
        else *H(x + 4) = 8;
    }
    leave(c, sv);
}

/* (x, y): with x's +93 set 0x089DEBE8. Otherwise a challenger coming in (0x0883B800) moves x's +4
   on with 0xB4 at +18; without one 0x08813770 and 0x08814384 run and x's first word moves on with
   +4 and +18 cleared. Both clear +20 and end with 0x08821F9C and 0x0880ABD4. */
SFA_FUNC(089DECE0, step_by_challenger_b) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[2];
    int32_t held = S8(x + 93);
    enter(c, x, sv);
    if (held != 0) call2(c, y, y, f_089DEBE8, 0x089DED80u);
    else {
        call2(c, x, y, f_0883B800, 0x089DED08u);
        if (c->r[2] != 0) {
            *H(x + 4) = (uint16_t)(*H(x + 4) + 1);
            *H(x + 18) = 0xB4;
            *H(x + 20) = 0;
            call0(c, f_08821F9C, 0x089DED2Cu);
            call0(c, f_0880ABD4, 0x089DED34u);
        } else {
            call0(c, f_08813770, 0x089DED44u);
            call0(c, f_08814384, 0x089DED4Cu);
            *H(x + 4) = 0;
            *H(x) = (uint16_t)(*H(x) + 1);
            *H(x + 18) = 0;
            *H(x + 20) = 0;
            call0(c, f_08821F9C, 0x089DED68u);
            call0(c, f_0880ABD4, 0x089DED70u);
        }
    }
    leave(c, sv);
}
