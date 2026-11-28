/* One more routine the fighters' frame leans on that no test scenario runs: what a fighter set
   to act by itself does when it could be hit. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088625C0(Cpu *c); void f_08849DF8(Cpu *c); void f_08821368(Cpu *c);

/* The mark (1, 2 or 7) goes into the fighter's four bits of a's +17. With `ask`, only when the
   routine at 0x088625C0 says so for the fighter, or the check of the other one does. */
static void mark(Cpu *c, uint32_t value, int ask, uint32_t back) {
    gaddr a = c->r[16], f = c->r[17];
    if (ask) {
        ARG(0) = f; c->r[31] = back; f_088625C0(c);
        ARG(0) = a;
        if (c->r[2] == 0) {
            ARG(1) = f; ARG(2) = c->r[18];
            c->r[31] = back + 0x14; f_08849DF8(c);
            if (c->r[2] == 0) return;
        }
    }
    ARG(0) = (uint32_t)S8(f + 337) << 2;
    ARG(1) = *B(a + 17);
    c->r[6] = value;
    ARG(0) = ARG(1) | (value << (ARG(0) & 31));
    *B(a + 17) = (uint8_t)ARG(0);
}

/* (a, f, g, side): by the side's setting in a (+4, and +6 when that is not 6):
     setting 6: a's +17 is cleared, and only with the fighter holding away (bit 0 of its
       buttons, turned when its +11 is set) mark 1 or, with its +696 set, mark 2, at once when
       its +127 is set.
     kind 0: after the fighter's +272 was set once, a count of 0x3C frames (a's +18 and +19) that
       starts again while +272 is set; while it runs mark 7; it ends when it runs out with the
       fighter not in step 1 with +6 other than 2.
     kind 2: mark 1; kind 3: mark 2 (each at once when the fighter's +127 is set).
     kind 4: a fighter in step 1 with +6 other than 2 and +117 at 0xFF, with +272 clear, picks
       mark 1 or 2 by a random bit (kept in its +696); any other fighter gets mark 7 on a
       random bit.
     kind 5: mark 7.
   A 0x30 frame keeping s0..s3. */
SFA_FUNC(0884AB58, self_acting_marks) {
    gaddr a = ARG(0), f = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = a;
    gaddr side = a + (ARG(3) & 0xFF);
    c->r[7] = *B(side + 4);
    *W(sp + 20) = s1;
    c->r[8] = 6;
    c->r[17] = f;
    *W(sp + 24) = s2; *W(sp + 28) = s3; *W(sp + 32) = ra;
    c->r[18] = ARG(2);
    int which = 0; /* 1 or 2: the mark to give, at once when the fighter's +127 is set */
    if (c->r[7] == 6) {
        *B(a + 17) = 0;
        uint32_t held = *H(f + 752);
        if (*B(f + 11) != 0) held = ~held;
        if ((held & 3 & 1) != 0) which = S8(f + 696) != 0 ? 2 : 1;
    } else {
        uint32_t kind = *B(side + 6);
        if (kind == 0) {
            uint32_t on = *B(a + 18);
            if (on == 0) {
                if (*W(f + 272) != 0) {
                    *B(a + 18) = 1;
                    *B(a + 19) = 0x3C;
                }
            } else {
                uint32_t left = *B(a + 19);
                if (left != 0) {
                    *B(a + 19) = (uint8_t)(left - 1);
                    left = *B(a + 19);
                }
                if (left == 0 && (*B(f + 5) != 1 || *B(f + 6) == 2)) {
                    *B(a + 18) = 0;
                } else {
                    if (*W(f + 272) != 0) *B(a + 19) = 0x3C;
                    mark(c, 7, 1, 0x0884ACD4u);
                }
            }
        } else if (kind == 2) {
            which = 1;
        } else if (kind == 3) {
            which = 2;
        } else if (kind == 4) {
            c->r[19] = 1;
            if (*B(f + 5) == 1 && *B(f + 6) != 2 && *B(f + 117) == 0xFF) {
                if (*W(f + 272) == 0) {
                    c->r[31] = 0x0884ADF4u; f_08821368(c);
                    if ((c->r[2] & 1) == 0) { *B(f + 696) = 1; which = 2; }
                    else { *B(f + 696) = 0; which = 1; }
                }
            } else {
                c->r[31] = 0x0884AE18u; f_08821368(c);
                if ((c->r[2] & 1) != 0) mark(c, 7, 1, 0x0884AC18u);
            }
        } else if (kind == 5) {
            mark(c, 7, 1, 0x0884AC18u);
        }
    }
    if (which == 1) mark(c, 1, S8(f + 127) == 0, 0x0884AD24u);
    else if (which == 2) mark(c, 2, S8(f + 127) == 0, 0x0884AD74u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}
