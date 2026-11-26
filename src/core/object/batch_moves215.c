/* The super meter gain (0x0886CA2C, 3,083 calls) and two frame-flag steps that throw hit sparks
   (0x088F0010, 0x08964384). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08880034(Cpu *c); void f_0886CBFC(Cpu *c); void f_0887C408(Cpu *c);

#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }

/* (fighter): a frame flag +76 steps +7 on, sets the guard pick +237 (1 with +81, else -1) and throws hit
   sparks 25 ahead / 62 up (0x08880034); ticked. */
static void flag_sparks(Cpu *c, uint32_t b0, uint32_t b1) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    int32_t k = S8(o + 76);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (k != 0) {
        uint32_t st = *B(o + 7);
        int32_t g = S8(o + 81);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 237) = g != 0 ? 1 : 0xFF;
        ARG(0) = o; ARG(1) = 0x19; ARG(2) = 0x3E; c->r[31] = b0; f_08880034(c);
    }
    call1(c, o, f_08861848, b1);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088F0010, move_flag_sparks) { flag_sparks(c, 0x088F0060u, 0x088F0068u); }
SFA_FUNC(08964384, move_flag_sparks_b) { flag_sparks(c, 0x089643D4u, 0x089643DCu); }

/* (fighter, amount): unless blocked (+136, +313, the match over, a zero amount or a knocked-out opponent)
   the meter changes. A gain (skipped when +792 bit 28 is set without +812) grows by half when locked or
   with no choice made, doubles with +789 = 1, then is scaled by 3/4 (at least 1); a loss is skipped with
   +789 = 2. On the clock (+56) the team meter is fed through 0x0886CBFC (mode 3 by side bit 1, else the
   partner pair 0x092E47D0 / 0x092E3EA0 copied across); else +366 moves within 0 - 144 with voice 0x31
   when it fills. */
SFA_FUNC(0886CA2C, meter_gain) {
    gaddr o = ARG(0);
    int32_t n = (int32_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    gaddr opp = *W(o + 92);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    if (S8(o + 136) != 0 || S8(o + 313) != 0 || S8(MATCH + 7) != 0 || n == 0 || S16(opp + 112) < 0) {
        c->r[16] = s0; frame_out(c, ra);
        return;
    }
    if (n > 0) {
        if ((*W(o + 792) & 0x10000000u) && S8(o + 812) == 0) { c->r[16] = s0; frame_out(c, ra); return; }
        if (S8(o + 371) != 0 || S8(o + 386) == -1) n += n >> 1;
        if (S8(o + 789) == 1) n <<= 1;
        n = (n >> 1) + (n >> 2);
        if (n == 0) n = 1;
    } else if (S8(o + 789) == 2) { c->r[16] = s0; frame_out(c, ra); return; }
    int32_t mode = S16(MATCH + 56);
    if (mode != 0) {
        int32_t side = S8(o + 337) & 2;
        int team;
        gaddr mate = 0, copy = 0;
        if (mode == 3) { team = side == 0; mate = 0x092E3EA0u; copy = 0x092E4338u; }
        else { team = side != 0; mate = 0x092E47D0u; copy = 0x092E4C68u; }
        if (team) {
            c->r[16] = mate;
            call2(c, mate, (uint32_t)n, f_0886CBFC, mode == 3 ? 0x0886CBDCu : 0x0886CB94u);
            *H(copy + 366) = (uint16_t)S16(mate + 366);
        } else call2(c, o, (uint32_t)n, f_0886CBFC, mode == 3 ? 0x0886CBC8u : 0x0886CBB0u);
    } else {
        int32_t old = S16(o + 366);
        int16_t v = (int16_t)(old + n);
        *H(o + 366) = (uint16_t)v;
        if (v < 0) *H(o + 366) = 0;
        else if (!(v < 144)) *H(o + 366) = 0x90;
        int32_t now = S16(o + 366);
        if (old != now && now == 0x90) {
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x31;
            c->r[31] = 0x0886CB64u; f_0887C408(c);
        }
    }
    c->r[16] = s0; frame_out(c, ra);
}
