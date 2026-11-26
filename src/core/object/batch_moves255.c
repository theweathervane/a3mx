/* A held throw that is mashed (0x0894D6E8): it holds the opponent (+92) while a count (+82) runs, button
   presses keep it going and can change its strength, and it lets go into 0x0894CBCC. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_088616EC(Cpu *c); void f_08861848(Cpu *c);
void f_088632C4(Cpu *c); void f_0886DC3C(Cpu *c); void f_08861900(Cpu *c); void f_08877258(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0894CBCC(Cpu *c); void f_0886DB2C(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static int paused(void) {
    const gaddr g = 0x092E1228u;
    return (S8(g + 13) | S8(g + 14) | S8(0x08E75910u + 38)) != 0;
}
/* the opponent let go of a pending flag (+592 -> +270), and the hold restarted at its animation's end */
static void keep(Cpu *c, gaddr o, gaddr p, uint32_t b0, uint32_t b1) {
    if (S8(p + 592) != 0) { *B(p + 270) = 1; *B(p + 592) = 0; }
    if (S8(o + 76) != 0 && S8(o + 74) == 1) {
        *B(o + 76) = 0;
        call1(c, o, f_0886DC3C, b0);
        call1(c, o, f_08837098, b1);
        if (S8(o + 551) == 0) *B(o + 551) = 0xFF;
    }
}
/* the release: +7 on, +262 cleared and move 2 x strength + 0x11 */
static void release(Cpu *c, gaddr o, uint32_t back) {
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 262) = 0;
    call2(c, o, (uint32_t)((s << 1) + 0x11) & 0xFFFF, f_088616EC, back);
}
/* +82 down: released at 0, else ticked */
static void count_down(Cpu *c, gaddr o, uint32_t bt) {
    int32_t t = S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) release(c, o, 0x0894D8E8u);
    else call1(c, o, f_08861848, bt);
}

/* (fighter): step 0: step 1, +262 = 1, the meter by strength + 3, 0x08837098, +551 cleared and move 2 x
   strength + 0x10. Step 1 waits for a frame flag, then step 2 with +82 = 0xF (with a choice made +82 =
   +193). Step 2, with no choice made: released while paused or when 0x088632C4 says so; else kept, and a
   press of strength 0..2 (+760 bits 4..6) with +82 under 3 x that strength counts down (released at 0),
   otherwise +82 = 0xF and a new strength restarts the move at it (+28 on by 10 x +77, 0x08861900); with
   no press +82 counts down. With a choice made it is released the same way, else kept and with +193 +82
   counts down, without it released when 0x08877258 (0x08A78D70) says so. Each step-2 frame ends with
   0x0886E3CC (8, 4). Step 3: once ended the opponent's +543 = 5 and 0x0894CBCC, else 0x0886DB2C and
   ticked. */
SFA_FUNC(0894D6E8, mashed_hold) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 92);
    c->r[17] = p;
    uint32_t st = *B(o + 7);
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[18] = 1;
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1;
        call2(c, o, (uint32_t)(S8(o + 194) + 3), f_0886CA2C, 0x0894D72Cu);
        call1(c, o, f_08837098, 0x0894D734u);
        int32_t s = S8(o + 194);
        *B(o + 551) = 0;
        call2(c, o, (uint32_t)((s << 1) + 0x10) & 0xFFFF, f_088616EC, 0x0894D750u);
        goto out;
    }
    if (st == 1) {
        if (S8(o + 77) == 0) { call1(c, o, f_08861848, 0x0894D7C0u); goto out; }
        *B(o + 7) = 2; *H(o + 82) = 0xF;
        if (S8(o + 373) != 0) { *H(o + 82) = (uint16_t)S8(o + 193); goto locked; }
    } else if (st == 3) {
        if (S8(o + 75) < 0) { *B(p + 543) = 5; call1(c, o, f_0894CBCC, 0x0894DAA8u); }
        else { call1(c, o, f_0886DB2C, 0x0894DAB8u); call1(c, o, f_08861848, 0x0894DAC0u); }
        goto out;
    } else if (st != 2) goto out;
    if (S8(o + 373) != 0) goto locked;
    /* no choice made */
    if (paused()) { release(c, o, 0x0894D8E8u); goto tail; }
    call1(c, o, f_088632C4, 0x0894D80Cu);
    if (c->r[2] != 0) { release(c, o, 0x0894D8E8u); goto tail; }
    keep(c, o, p, 0x0894D84Cu, 0x0894D854u);
    {
        uint32_t h = *H(o + 760);
        int32_t n = S16(o + 82);
        if ((h & 0x70) == 0) { count_down(c, o, 0x0894D978u); goto tail; }
        int32_t lv = (h & 0x10) ? 0 : (h & 0x20) ? 1 : 2;
        if (n < lv * 3) {
            *H(o + 82) = (uint16_t)(n - 1);
            if ((int16_t)(n - 1) == 0) release(c, o, 0x0894D8E8u);
            goto tail;
        }
        *H(o + 82) = 0xF;
        if (S8(o + 194) == lv) { call1(c, o, f_08861848, 0x0894D950u); goto tail; }
        *B(o + 194) = (uint8_t)lv;
        int32_t s = S8(o + 194);
        int32_t f = S8(o + 77);
        c->r[17] = (uint32_t)f;
        call2(c, o, (uint32_t)((s << 1) + 0x10) & 0xFFFF, f_088616EC, 0x0894D920u);
        *W(o + 28) = *W(o + 28) + (uint32_t)(f * 10);
        call1(c, o, f_08861900, 0x0894D940u);
        goto tail;
    }
locked:
    if (paused()) { release(c, o, 0x0894DA70u); goto tail; }
    call1(c, o, f_088632C4, 0x0894D990u);
    if (c->r[2] != 0) { release(c, o, 0x0894DA70u); goto tail; }
    keep(c, o, p, 0x0894D9D0u, 0x0894D9D8u);
    if (S8(o + 193) != 0) count_down(c, o, 0x0894DA18u);
    else {
        ARG(0) = o; ARG(1) = 0x08A78D70u; c->r[31] = 0x0894DA30u; f_08877258(c);
        if (c->r[2] != 0) release(c, o, 0x0894D8E8u);
        else call1(c, o, f_08861848, 0x0894DA40u);
    }
tail:
    ARG(0) = o; ARG(1) = 8; ARG(2) = 4; c->r[31] = 0x0894DA80u; f_0886E3CC(c);
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
