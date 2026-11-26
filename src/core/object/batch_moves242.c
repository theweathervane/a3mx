/* Specials that throw one or two projectiles on a frame flag, like 0x089292B0 in batch_moves241. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08822D2C(Cpu *c); void f_089291EC(Cpu *c); void f_088EF8C8(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static gaddr spawn(Cpu *c, gaddr o, uint32_t kind, uint32_t back) {
    ARG(0) = o; ARG(1) = kind; ARG(2) = 0; c->r[31] = back; f_08822D2C(c);
    return c->r[2];
}
static void place(Cpu *c, gaddr o, gaddr p, uint32_t back) {
    ARG(0) = o; ARG(1) = p; ARG(2) = 0; c->r[31] = back; f_089291EC(c);
}
/* the projectile's fields from its owner; `s_add` raises its strength, `b139` is its +139 (-1 leaves it) */
static void shot_fields(gaddr p, gaddr o, int32_t s_add, int32_t b139) {
    *B(p + 122) = *B(o + 122); *B(p + 121) = *B(o + 121); *B(p + 346) = (uint8_t)(S8(o + 194) + s_add);
    *B(p + 255) = 0; *B(p + 80) = *B(o + 80); *B(p + 347) = 0;
    *B(p + 14) = *B(o + 14); *B(p + 11) = *B(o + 11); *B(p + 12) = *B(o + 12);
    *H(p + 86) = (uint16_t)S8(o + 337);
    *W(p + 164) = *W(o + 164); *H(p + 38) = *H(o + 38); *B(p + 13) = *B(o + 13);
    *H(p + 18) = *H(o + 18); *H(p + 22) = *H(o + 22); *H(p + 112) = 0;
    if (b139 >= 0) *B(p + 139) = (uint8_t)b139;
    *W(p + 88) = o;
}
/* the owner keeps its latest projectile and counts them */
static void link_shot(gaddr o, gaddr p) {
    *W(o + 228) = p;
    *B(o + 536) = (uint8_t)(*B(o + 536) + 1);
}
static void shot_from(gaddr p, gaddr o, int32_t s_add, int32_t b139) { shot_fields(p, o, s_add, b139); link_shot(o, p); }
static int second_shot(gaddr o) { return S8(o + 251) != 0 && S16(o + 286) >= S16(o + 662); }

/* (fighter): ticked; on a frame flag +7 on and a projectile (object 3: +132 cleared, +260 = 8, +311 = 2,
   +312 = 8); with +251 and +286 at least +662 a second one carrying +662. */
SFA_FUNC(08900C00, special_shots_3) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x08900C1Cu);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        gaddr p = spawn(c, o, 3, 0x08900C44u);
        if (p != 0) {
            shot_from(p, o, 0, -1);
            *H(p + 132) = 0;
            c->r[18] = 8; c->r[17] = 2;
            *B(p + 260) = 8; *B(p + 311) = 2; *B(p + 312) = 8;
            if (second_shot(o)) {
                gaddr p2 = spawn(c, o, 3, 0x08900D24u);
                if (p2 != 0) {
                    shot_from(p2, o, 0, -1);
                    *H(p2 + 132) = 0;
                    *B(p2 + 280) = (uint8_t)S16(o + 662);
                    *B(p2 + 260) = 8; *B(p2 + 311) = 2; *B(p2 + 312) = 8;
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked; on frame flag 2 +7 on, +82 = 5 and a projectile (object 0xB, strength + 3, +139 = 2),
   0x089291EC; with +251 and +286 at least +662 a second one carrying +662. */
SFA_FUNC(08929494, special_shots_b3) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x089294ACu);
    c->r[17] = 2;
    if (S8(o + 77) == 2) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *H(o + 82) = 5;
        gaddr p = spawn(c, o, 0xB, 0x089294E0u);
        if (p != 0) {
            shot_from(p, o, 3, 2);
            place(c, o, p, 0x08929590u);
            if (second_shot(o)) {
                gaddr p2 = spawn(c, o, 0xB, 0x089295BCu);
                if (p2 != 0) {
                    shot_from(p2, o, 3, 2);
                    *B(p2 + 280) = (uint8_t)S16(o + 662);
                    place(c, o, p2, 0x08929674u);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked; on a frame flag +7 on and a projectile (object 7: +260 = 7, +311 = 2, +312 = 8); with
   +251 and +286 at least +662 a second one carrying +662. A 0x30 frame keeping s0..s3. */
SFA_FUNC(0899D958, special_shots_7) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0899D978u);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        gaddr p = spawn(c, o, 7, 0x0899D9A0u);
        if (p != 0) {
            shot_from(p, o, 0, -1);
            c->r[19] = 7; c->r[18] = 2; c->r[17] = 8;
            *B(p + 260) = 7; *B(p + 311) = 2; *B(p + 312) = 8;
            if (second_shot(o)) {
                gaddr p2 = spawn(c, o, 7, 0x0899DA80u);
                if (p2 != 0) {
                    shot_from(p2, o, 0, -1);
                    *B(p2 + 260) = 7; *B(p2 + 311) = 2; *B(p2 + 312) = 8;
                    *B(p2 + 280) = (uint8_t)S16(o + 662);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* the projectile set up by 0x088EF8C8, its strength raised by 0xB in stance 10, then kept by its owner */
static void shot_ef8c8(Cpu *c, gaddr o, gaddr p, uint32_t back) {
    shot_fields(p, o, 0, -1);
    ARG(0) = o; ARG(1) = p; c->r[31] = back; f_088EF8C8(c);
}
static void shot_stance(gaddr o, gaddr p) {
    if (S8(o + 236) == 10) *B(p + 346) = (uint8_t)(S8(p + 346) + 0xB);
    link_shot(o, p);
}

/* (fighter): ticked; on a frame flag +7 on, +82 = 3 and a projectile (object 0) through 0x088EF8C8; with
   +251 and +286 at least +662 a second one carrying +662. */
SFA_FUNC(088F1F20, special_shots_0) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x088F1F3Cu);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *H(o + 82) = 3;
        gaddr p = spawn(c, o, 0, 0x088F1F6Cu);
        c->r[18] = p;
        if (p != 0) {
            shot_ef8c8(c, o, p, 0x088F2004u);
            c->r[17] = 0xA;
            shot_stance(o, p);
            if (second_shot(o)) {
                gaddr p2 = spawn(c, o, 0, 0x088F2058u);
                c->r[18] = p2;
                if (p2 != 0) {
                    shot_ef8c8(c, o, p2, 0x088F20F0u);
                    *B(p2 + 280) = (uint8_t)S16(o + 662);
                    shot_stance(o, p2);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
