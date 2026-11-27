/* More of what the new leaves set free: an air move's way down with its attack and throw, and the
   block's start (standing or crouching, or the paid escape for the cpu). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867C30(Cpu *c); void f_08984CF8(Cpu *c); void f_0886B100(Cpu *c); void f_08984D38(Cpu *c);
void f_08987118(Cpu *c); void f_08987404(Cpu *c); void f_08861848(Cpu *c); void f_08984DF4(Cpu *c);
void f_08863E1C(Cpu *c); void f_0885D584(Cpu *c); void f_08861724(Cpu *c); void f_08863940(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (fighter): an air move's way down. Falling, a player on a punch (or when 0x08867C30 (0x2B) passes)
   goes to the next step: the attack 0x08984CF8, or with a direction held and the throw check
   0x0886B100 (0x2E) passing, the throw 0x08984D38. The cpu does the same within 40 of the ground, and
   higher up only throws. Otherwise 0x08987118 and 0x08987404, then ticked until 32 above the ground,
   where it is set down for the next step (0x08984DF4). */
SFA_FUNC(08984B70, air_move_down) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t vy = (int32_t)*W(o + 100);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (vy < 0) {
        if (S8(o + 373) == 0) {
            int go = (*H(o + 760) & 0x70) != 0;
            if (!go) {
                call2(c, o, 0x2B, f_08867C30, 0x08984BACu);
                go = c->r[2] != 0;
            }
            if (go) {
                uint8_t step = *B(o + 7);
                uint32_t stick = *H(o + 290);
                *B(o + 7) = (uint8_t)(step + 1);
                if ((stick & 7) == 0) call1(c, o, f_08984CF8, 0x08984BD8u);
                else {
                    call2(c, o, 0x2E, f_0886B100, 0x08984BE8u);
                    if (c->r[2] != 0) call1(c, o, f_08984D38, 0x08984BF8u);
                    else call1(c, o, f_08984CF8, 0x08984C08u);
                }
                goto out;
            }
        } else if (S16(o + 132) + 0x28 < S16(o + 22)) {
            call2(c, o, 0x2E, f_0886B100, 0x08984C6Cu);
            if (c->r[2] != 0) { call1(c, o, f_08984D38, 0x08984CB4u); goto out; }
        } else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x2E, f_0886B100, 0x08984C3Cu);
            if (c->r[2] != 0) call1(c, o, f_08984D38, 0x08984C4Cu);
            else call1(c, o, f_08984CF8, 0x08984C5Cu);
            goto out;
        }
    }
    call1(c, o, f_08987118, 0x08984C7Cu);
    call1(c, o, f_08987404, 0x08984C84u);
    {
        int32_t ground = S16(o + 132);
        if (ground - 32 < S16(o + 22)) call1(c, o, f_08861848, 0x08984CA4u);
        else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *H(o + 22) = (uint16_t)ground;
            *B(o + 73) = 0;
            *H(o + 20) = 0;
            call1(c, o, f_08984DF4, 0x08984CE8u);
        }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the block's start. When 0x08863E1C (1) says so, the next step turned away from +134 and
   0x0885D584. Ticked with no vitality left or something stored (+816). A player still holding back
   (+752 bit 0) starts the crouching block (+696 set, animation 15) with down held, else the standing
   one (animation 12); ticked without back. The cpu with a full meter (0x90), a choice other than -1
   and +500 takes the paid escape (0x08863940); else crouches when the opponent's +310 is set or it
   crouches on the ground, and stands otherwise. A frame keeping s0..s2. */
SFA_FUNC(0885D3F8, block_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    gaddr other = *W(o + 92);
    c->r[18] = other; c->r[17] = 1;
    *W(c->r[29] + 28) = ra;
    call2(c, o, 1, f_08863E1C, 0x0885D420u);
    if (c->r[2] != 0) {
        uint8_t step = *B(o + 7);
        int32_t side = S8(o + 134);
        *B(o + 7) = (uint8_t)(step + 1);
        *B(o + 11) = (uint8_t)((side ^ 1) & 1);
        call1(c, o, f_0885D584, 0x0885D44Cu);
    } else if (S16(o + 112) < 0 || *W(o + 816) != 0) call1(c, o, f_08861848, 0x0885D470u);
    else if (S8(o + 373) == 0) {
        uint32_t stick = *H(o + 752);
        if (!(stick & 1)) call1(c, o, f_08861848, 0x0885D4CCu);
        else if (stick & 4) { *B(o + 696) = 1; call2(c, o, 0xF, f_08861724, 0x0885D4A8u); }
        else { *B(o + 696) = 0; call2(c, o, 0xC, f_08861724, 0x0885D4BCu); }
    } else if (S16(o + 366) == 0x90 && S8(o + 386) != -1 && S8(o + 500) != 0)
        call1(c, o, f_08863940, 0x0885D50Cu);
    else if (S8(other + 310) != 0) { *B(o + 696) = 1; call2(c, o, 0xF, f_08861724, 0x0885D52Cu); }
    else if (S8(other + 73) == 0 && S8(other + 696) != 0) {
        *B(o + 696) = 1;
        call2(c, o, 0xF, f_08861724, 0x0885D558u);
    } else { *B(o + 696) = 0; call2(c, o, 0xC, f_08861724, 0x0885D56Cu); }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
