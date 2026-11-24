/* Small steps (0.146.0): the 1 function of 9 - 100 instructions made ready by the batch before and the
   first 3 of 101 - 200. The camera edges of fighters 1 - 3, the hit checks between the objects in the
   two hit lists, the network matching started by mode, and a banner's fade by kind. Offsets stay
   numbers until something shows what they are. What the original leaves in a0 - t3 at a call is set
   as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME       0x08E75910u
#define FIGHTER0   0x092E3EA0u
#define FIGHTER1   0x092E4338u
#define FIGHTER2   0x092E47D0u
#define HITTERS    0x09517700u   /* the objects that hit, by count 0x095177C0 */
#define HITTABLE   0x09517760u   /* the objects that can be hit, by count 0x095177C1 */
#define NET_THREAD 0x09523740u
#define BANNER_ON  0x09525E00u   /* u8: a banner is shown; its object at 0x09525E04 */

void f_0881E8B0(Cpu *c);   /* edges_of_three */
void f_088739CC(Cpu *c);   /* boxes_overlap */
void f_088743CC(Cpu *c);   /* step_088743CC */
void f_088D595C(Cpu *c);   /* net_state_3_8_or_10 */
void f_088233BC(Cpu *c);   /* object_free_list_3 */

/* (left, right): edges_of_three of fighters 1, 2 and 3. */
SFA_FUNC(0881E728, edges_of_fighters_1_2_3) {
    uint32_t left = ARG(0), right = ARG(1);
    c->r[29] -= 0x30;
    c->r[8] = right; c->r[7] = left;
    c->r[4] = FIGHTER0; c->r[5] = FIGHTER1; c->r[6] = FIGHTER2;
    f_0881E8B0(c);
    c->r[29] += 0x30;
}

/* An object in a hit list that takes part: not +270, not kind 0x17 / 0x22 (+2), running (+0 = 1), and
   a box number at its +256's first byte. */
static uint32_t hit_box(gaddr o) {
    if (S8(o + 270) != 0) return 0;
    uint32_t k = *B(o + 2);
    if (k == 0x17 || k == 0x22 || *B(o) != 1) return 0;
    return *B(*W(o + 256));
}

/* (): each object of the hitters' list (last first) against the hittable list's (last first): the
   first other one whose box (+208, 8 bytes a box) overlaps its own runs the hitter's step by +255. */
SFA_FUNC(08874224, hit_checks) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23], s30 = c->r[30];
    c->r[29] -= 0x40;
    gaddr slot = c->r[29] + 16;
    int32_t n = *B(0x095177C0u);
    if (n != 0 && *B(0x095177C1u) != 0) {
        n--;
        *W(slot) = HITTERS + (uint32_t)n * 4;
        c->r[23] = HITTABLE; c->r[22] = 0x17; c->r[21] = 0x22; c->r[20] = 1;
        for (; n >= 0; n--) {
            c->r[30] = (uint32_t)n;
            gaddr a = *W(*W(slot));
            c->r[18] = a;
            uint32_t ka = hit_box(a);
            if (ka != 0) {
                c->r[19] = ka * 8;
                for (int32_t j = (int32_t)*B(0x095177C1u) - 1; j >= 0; j--) {
                    c->r[17] = (uint32_t)j;
                    gaddr b = *W(HITTABLE + (uint32_t)j * 4);
                    c->r[16] = b;
                    if (a == b) continue;
                    uint32_t kb = hit_box(b);
                    if (kb == 0) continue;
                    c->r[4] = a; c->r[5] = b;
                    c->r[6] = *W(a + 208) + ka * 8;
                    c->r[7] = *W(b + 208) + kb * 8;
                    f_088739CC(c);
                    if (c->r[2] == 0) continue;
                    c->r[4] = a; c->r[5] = b; f_088743CC(c);
                    break;
                }
            }
            *W(slot) = *W(slot) - 4;
        }
    }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23; c->r[30] = s30;
}

/* (mode, opt, opt_len): ad hoc matching for modes 1 - 3 (else -1): initialised, made (peers from the
   network state, port 0x22B, 0x800 buffer, 5 s hello, 10 s ping, 3 tries, 0.5 s, the mode's handler
   0x088DE664 / 0x088DE680 / 0x088DE69C) into 0x09523748 and started with the option. A failing step's
   result. */
SFA_FUNC(088DE734, net_matching_start) {
    int32_t mode = (int32_t)ARG(0);
    uint32_t opt = ARG(1), opt_len = ARG(2);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x50;
    c->r[19] = 1; c->r[18] = (uint32_t)mode; c->r[17] = opt; c->r[16] = opt_len;
    if (mode < 1 || mode > 3) RETURN(0xFFFFFFFFu);
    else {
        int32_t r = os_net_adhoc_matching_init(c, 0x8000);
        if (r >= 0) {
            static const uint32_t handler[3] = { 0x088DE664u, 0x088DE680u, 0x088DE69Cu };
            f_088D595C(c);
            r = os_net_adhoc_matching_create(c, mode, (int32_t)c->r[2], 0x22B, 0x800, 0x4C4B40, 0x989680, 3, 0x7A120,
                                             handler[mode - 1]);
            if (r >= 0) {
                *W(NET_THREAD + 8) = (uint32_t)r;
                r = os_net_adhoc_matching_start(c, r, 0x10, 0x2000, 0x10, 0, (int32_t)opt_len, opt);
            }
        }
        RETURN((uint32_t)r);
    }
    c->r[29] += 0x50;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (banner): its fade by kind (+92 - 16 through the table 0x08A01930): up by 8 (to 0xFF), to 0x80 (and
   gone with the banner kind 0x1F), down by 8 (to 0), down by 8 ending the banner (0x09525E00), or up by
   6 (gone once its +124's first byte is 0); at the end GAME +93 / +94 = 0 / 1. Gone (+93 cleared,
   freed) whenever no banner is shown. */
SFA_FUNC(089DDCD4, banner_fade_step) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t k = (uint32_t)(int32_t)(int8_t)(*B(o + 92) - 16);
    int check_on = 1;
    if (k < 0x14u) {
        uint32_t to = *W(0x08A01930u + k * 4);
        uint32_t v = *B(o + 136);
        switch (to) {
        case 0x089DDD20u:
            if (v == 0xFF) { *B(GAME + 93) = 0; *B(GAME + 94) = 1; }
            else *B(o + 136) = (uint8_t)((int32_t)v < 248 ? v + 8 : 0xFF);
            break;
        case 0x089DDD5Cu:
            if (v != 0x80) *B(o + 136) = (uint8_t)(v + 8);
            if (*B(*W(BANNER_ON + 4) + 92) == 0x1F) { c->r[4] = o; f_088233BC(c); }
            break;
        case 0x089DDD9Cu:
            if (v == 0) { *B(GAME + 93) = 0; *B(GAME + 94) = 1; }
            else *B(o + 136) = (uint8_t)(v - 8);
            break;
        case 0x089DDDC0u:
            if (v == 0) { *B(GAME + 93) = 0; *B(GAME + 94) = 1; *B(BANNER_ON) = 0; }
            else *B(o + 136) = (uint8_t)((int32_t)v < 8 ? 0 : v - 8);
            break;
        case 0x089DDDFCu: {
            if (v == 0xFF) { *B(GAME + 93) = 0; *B(GAME + 94) = 1; }
            else *B(o + 136) = (uint8_t)((int32_t)v < 248 ? v + 6 : 0xFF);
            gaddr w = *W(o + 124);
            if (w == 0 || *B(w) == 0) { *B(o + 93) = 0; c->r[4] = o; f_088233BC(c); }
            check_on = 0;
            break;
        }
        case 0x089DDE64u: break;
        default: rt_bad_jump(c, to, 0x089DDD18u); return;
        }
    }
    if (check_on && *B(BANNER_ON) == 0) { *B(o + 93) = 0; c->r[4] = o; f_088233BC(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
