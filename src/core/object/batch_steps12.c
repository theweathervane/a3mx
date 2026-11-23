/* Small steps (0.141.0): the 5 functions of 9 - 60 instructions made ready by the batches before and
   the first 8 of 61 - 100. A fighter's reset when free or past its gauge, a CPU script's special by
   ISM, two boxes overlapping, the gauge's digits, the fight frame's objects, an object turning toward
   the nearer fighter, the network members advanced and the network thread's loop. Offsets stay
   numbers until something shows what they are. What the original leaves in a0 - t1 at a call is set
   as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define NET_STATE 0x09523920u
#define NET_THREAD 0x09523740u

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_0881EBCC(Cpu *c);   /* height_of_three */
void f_088214E8(Cpu *c);   /* object_draw_if_on_screen_margin */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_0884E518(Cpu *c);   /* frame_by_78 */
void f_0884E570(Cpu *c);   /* random_82 */
void f_0884E5A0(Cpu *c);   /* turn_toward_nearest */
void f_0889DB8C(Cpu *c);   /* place_kind */
void f_0889D830(Cpu *c);   /* rows_by_character */
void f_0886C164(Cpu *c);   /* fighter_may_start */
void f_0886C08C(Cpu *c);   /* fighter_try_reset */
void f_088DD3DC(Cpu *c);   /* session_end_unless_waiting */
void f_088DD418(Cpu *c);   /* session_member_by_count */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_08823220(Cpu *c);   /* record_free_5 */
void f_0887B528(Cpu *c);   /* fighter_cpu_state_from_word */
void f_0887B504(Cpu *c);   /* fighter_run_own_routine */
void f_0886B468(Cpu *c);   /* battle_136_or_140 */
void f_0886DC3C(Cpu *c);   /* fighter_clear_644 */
void f_08877F44(Cpu *c);   /* fighter_to_routine_1 */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_088DF0B8(Cpu *c);   /* adhoc_matching_end */
void f_088DF400(Cpu *c);   /* adhoc_close */
void f_088DF46C(Cpu *c);   /* adhoc_take_flags */
void f_088884C8(Cpu *c);   /* step_088884C8 */
void f_088891CC(Cpu *c);   /* step_088891CC */
void f_08835B30(Cpu *c);   /* mark_slot_138 */

/* ---------------------------------------------------------------- objects */

/* (out): height_of_three of fighters 1, 2 and 4 into *out. */
SFA_FUNC(0881EAE0, height_of_fighters_1_2_4) {
    uint32_t out = ARG(0);
    c->r[29] -= 0x20;
    c->r[4] = FIGHTER0; c->r[5] = FIGHTER1; c->r[6] = FIGHTER3; c->r[7] = out;
    f_0881EBCC(c);
    c->r[29] += 0x20;
}

/* (object): unless BATTLE +37 / +52: while +82 counts down, at 0 the animation by +3 from 0x08A150CC;
   then every +74 frames it turns one step toward the nearer fighter (and takes the frame for it);
   with +82 at 0, animated while +75 is not below 0, else +78 = 8 with its frame and a random +82.
   Drawn with a margin of 0x20. */
SFA_FUNC(0884E420, turning_watcher) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if ((S8(BATTLE + 37) | S8(BATTLE + 52)) == 0) {
        int16_t n = S16(o + 82);
        if (n != 0) {
            n = (int16_t)(n - 1);
            *H(o + 82) = (uint16_t)n;
            if (n == 0) {
                c->r[4] = o; c->r[5] = *W(0x08A150CCu + *B(o + 3) * 4); f_0882196C(c);
            } else {
                int8_t k = S8(o + 74);
                if (k != 0) *B(o + 74) = (uint8_t)(k - 1);
                else {
                    c->r[4] = o; f_0884E5A0(c);
                    if (c->r[2] != 0) { c->r[4] = o; f_0884E518(c); }
                }
            }
        } else if (S8(o + 75) >= 0) {
            c->r[4] = o; f_088219B0(c);
        } else {
            *B(o + 78) = 8;
            c->r[4] = o; f_0884E518(c);
            c->r[4] = o; f_0884E570(c);
        }
    }
    c->r[4] = o; c->r[5] = 0x20; f_088214E8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): placed (kind 0 at 0x164, 0), +148 = 1, animation 0x08A29880, its rows by the owner's
   character, rising (+100 = -0x10000). */
SFA_FUNC(0889D7B8, rows_object_start) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = 0x164; c->r[7] = 0; f_0889DB8C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 1;
    gaddr own = *W(o + 88);
    c->r[17] = own;
    c->r[4] = o; c->r[5] = *W(0x08A29880u); f_0882196C(c);
    c->r[4] = o; c->r[5] = own; f_0889D830(c);
    *W(o + 100) = 0xFFFF0000u;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): a row object's start: +128 counts on, +93 set, place 0 / 0xF0, size 0x180 x 0x80, colour
   by +92 - 16 from 0x08AB6F4A into +56; its twelve shades (+36 - +51 but +40, +41, +46, +47) and +136
   by +92 - 16 through the table 0x08A018E0 (0xFF / 0 / 0x80 pairs), else the number itself. */
SFA_FUNC(089DDBD0, row_start_by_92) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1; *B(o + 2) = 0; *B(o + 55) = 7; *B(o + 3) = 0;
    *H(o + 6) = 0; *H(o + 10) = 0xF0; *H(o + 12) = 0; *H(o + 24) = 0;
    *H(o + 14) = 0x180; *H(o + 26) = 0x80;
    uint32_t k = (uint32_t)(int32_t)(int8_t)(*B(o + 92) - 16);
    *B(o + 56) = *B(0x08AB6F4Au + k);
    uint32_t v = k;
    if (k < 0x14u) {
        uint32_t to = *W(0x08A018E0u + k * 4);
        v = k << 2;
        switch (to) {
        case 0x089DDC5Cu: v = 0xFFFF; *B(o + 136) = 0;    break;
        case 0x089DDC68u: v = 0x8000; *B(o + 136) = 0xFF; break;
        case 0x089DDC78u: v = 0x8000; *B(o + 136) = 0;    break;
        case 0x089DDC84u: v = 0xFFFF; *B(o + 136) = 0xFF; break;
        case 0x089DDC94u: v = 0x8000; *B(o + 136) = 0x80; break;
        case 0x089DDCA0u: break;
        default: rt_bad_jump(c, to, 0x089DDC54u); return;
        }
    }
    static const uint8_t at[] = { 39, 38, 37, 36, 45, 44, 43, 42, 51, 50, 49, 48 };
    for (unsigned i = 0; i < sizeof at; i++) *B(o + at[i]) = (uint8_t)v;
}

/* (object): +4 0 starts it (lit, +12 -1, +13 0x29, +86 0xF, animation by +3 from 0x08A15F24); 1 moves
   it by +96 (unless BATTLE +37 / +52 / +15) until x < -192, then 2; 2 frees it. Drawn while 0 / 1. */
SFA_FUNC(08852A7C, drift_left_object) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    if (st == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0xFF; *B(o + 13) = 0x29;
        *H(o + 86) = 0xF;
        c->r[4] = o; c->r[5] = *W(0x08A15F24u + *B(o + 3) * 4); f_0882196C(c);
        c->r[4] = o; f_08821424(c);
    } else if (st == 1) {
        if (S16(o + 18) < -192) *B(o + 4) = 2;
        else {
            if ((S8(BATTLE + 37) | S8(BATTLE + 52) | S8(BATTLE + 15)) == 0) *W(o + 16) += *W(o + 96);
            c->r[4] = o; f_08821424(c);
        }
    } else if (st == 2) {
        c->r[4] = o; f_08823220(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (game): the fight frame's objects: the routine by 0x0950FF40 from 0x08A0F954, then the 32 records
   of 0x94 at 0x092E2B88 in use (+90) stepped by kind (+91 3, or 4 with +92 3), then each fighter's
   slot marks. */
SFA_FUNC(088348F8, fight_frame_objects) {
    gaddr g = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    c->r[29] -= 0x30;
    c->r[16] = g;
    uint32_t fn = *W(0x08A0F954u + *B(0x0950FF40u) * 4);
    c->r[4] = g; c->r[6] = 0x0950FF40u; c->r[7] = fn;     /* as the original leaves them */
    rt_call_indirect(c, fn);
    c->r[18] = 3; c->r[17] = 4;
    for (int i = 0; i < 32; i++) {
        gaddr r = 0x092E2B88u + (uint32_t)i * 0x94;
        c->r[19] = (uint32_t)i; c->r[20] = r;
        if (*B(r + 90) == 0) continue;
        if (*B(r + 91) == 3) { c->r[4] = r; f_088884C8(c); }
        if (*B(r + 91) == 4 && *B(r + 92) == 3) { c->r[4] = r; f_088891CC(c); }
    }
    static const gaddr fs[] = { FIGHTER0, FIGHTER1, FIGHTER2, FIGHTER3 };
    for (int i = 0; i < 4; i++) { c->r[4] = g; c->r[5] = fs[i]; f_08835B30(c); }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
}

/* (o): the gauge's digits from BATTLE +92's record: +346 * 10 / 128 (tens) and +348 (ones); when they
   change, +16 cleared, kept at +73 / +112, and the 13 rows of 0x08A29888 (6 bytes, from +0x96 down) given
   their glyph halves from 0x08A24B28 / 0x08A24B14 (0x4000 once the count is spent); +154 bit 15 set. */
SFA_FUNC(088812AC, gauge_digits) {
    gaddr o = ARG(0), rec = *W(BATTLE + 92);
    int16_t a = S16(rec + 346);
    int16_t tens = (int16_t)((int32_t)(int16_t)(a * 10) >> 7);
    int16_t ones = S16(rec + 348);
    if (S8(o + 73) == tens && S16(o + 112) == ones) return;
    *W(o + 16) = 0;
    *B(o + 73) = (uint8_t)tens;
    *H(o + 112) = (uint16_t)ones;
    gaddr base = 0x08A29888u, p = base + 0x96;
    for (int16_t n = 0xC; ; ) {
        gaddr q = p - 6;
        if (ones < 0) { *H(p + 4) = 0x4000; *H(q + 4) = 0x4000; }
        else {
            *H(p + 4) = *H(0x08A24B28u + (uint32_t)(int32_t)tens * 2);
            *H(q + 4) = *H(0x08A24B14u + (uint32_t)(int32_t)ones * 2);
            tens = (int16_t)(tens - 1);
            if (tens < 0) { ones = (int16_t)(ones - 1); tens = 9; }
        }
        p = q - 6;
        int16_t was = n; n = (int16_t)(n - 1);
        if (was == 0) break;
    }
    *H(base + 154) |= 0x8000;
}

/* ---------------------------------------------------------------- fighters */

/* (a, b, box a, box b): 1 when the two boxes (x, y, half width, half height; x turned with facing +11)
   placed at the fighters overlap, else 0. */
SFA_FUNC(088739CC, boxes_overlap) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    int16_t xa = *B(a + 11) ? (int16_t)(S16(a + 18) - S16(ba)) : (int16_t)(S16(ba) + S16(a + 18));
    int16_t xb = *B(b + 11) ? (int16_t)(S16(b + 18) - S16(bb)) : (int16_t)(S16(bb) + S16(b + 18));
    int16_t dx = (int16_t)(xb - xa);
    if (dx < 0) dx = (int16_t)-dx;
    int ok = 0;
    if (!((int16_t)(S16(ba + 4) + S16(bb + 4)) < dx)) {
        int16_t ya = (int16_t)(S16(ba + 2) + S16(a + 22)), yb = (int16_t)(S16(bb + 2) + S16(b + 22));
        int16_t dy = (int16_t)(yb - ya);
        if (dy < 0) dy = (int16_t)-dy;
        ok = !((int16_t)(S16(ba + 6) + S16(bb + 6)) < dy);
    }
    RETURN(ok);
}

/* The two checks before a reset: in routine 1 / 1 / 2 / 0 straight to fighter_try_reset (take, 0);
   else not with +73, and only when fighter_may_start (take, 1) agrees, then fighter_try_reset
   (take, 1). 0 otherwise. */
static void reset_when_free(Cpu *c, gaddr f, uint32_t take) {
    uint32_t arg = 0;
    if (!(*B(f + 4) == 1 && *B(f + 5) == 1 && *B(f + 6) == 2 && *B(f + 7) == 0)) {
        arg = 1;
        c->r[17] = 1;
        if (S8(f + 73) != 0) { RETURN(0); return; }
        c->r[4] = f; c->r[5] = take; c->r[6] = 1; f_0886C164(c);
        if (c->r[2] == 0) { RETURN(0); return; }
    }
    c->r[4] = f; c->r[5] = take; c->r[6] = arg; f_0886C08C(c);
}

/* (fighter): reset_when_free with take 0. */
SFA_FUNC(0886BF84, fighter_reset_when_free) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[18] = 0; c->r[17] = 0;
    reset_when_free(c, f, 0);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (fighter): with its gauge (+366) at 0x30 (ISM 0), 0x90 (ISM -1) or 0x48 and +251 clear,
   reset_when_free with take 1; else 0. */
SFA_FUNC(0886C6EC, fighter_reset_by_gauge) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[18] = 1; c->r[17] = 0;
    int32_t ism = S8(f + 386);
    int32_t lim = ism == 0 ? 0x30 : ism == -1 ? 0x90 : 0x48;
    if (S16(f + 366) < lim || S8(f + 251) != 0) RETURN(0);
    else reset_when_free(c, f, 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* The script's special started: routine 1, part 8, +482 = `kind`, +483, facing from +697, +235 set,
   +696 / +123 cleared. */
static void special_start(gaddr f, uint32_t kind, uint32_t second) {
    *B(f + 482) = (uint8_t)kind; *B(f + 483) = (uint8_t)second;
    *B(f + 4) = 1; *B(f + 6) = 8; *B(f + 7) = 0; *B(f + 5) = 0;
    *B(f + 235) = 1;
    *B(f + 11) = (uint8_t)S8(f + 697);
}

/* (fighter, script): a CPU script command: +490 = the next s16. Without +488: its CPU state from it;
   V-ISM (+386 1) goes to routine 1 with +483 = 8; with the gauge (+366) at 48 (and at exactly 0x90
   unless A-ISM, which also sets +194 = 2) and its own routine run, a super (part 8, +482 5, the battle
   record's +356 = 2 when it stood in routine 1 / 1 / 2 / 0). With +488: V-ISM at 72 gauge, +251 clear
   and its own routine run start the V-ISM's activation by +491 (+548, +662, +664, +270). Else the
   script read on. Returns past the command. */
SFA_FUNC(0887BED0, script_special_by_ism) {
    gaddr f = ARG(0), p = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    c->r[18] = p + 2; c->r[17] = 1; c->r[16] = f; c->r[19] = p + 2;
    int read = 0;
    *H(f + 490) = (uint16_t)S16(p);
    if (S8(f + 488) == 0) {
        c->r[4] = f; c->r[5] = *H(f + 490); f_0887B528(c);
        int32_t ism = S8(f + 386);
        if (ism == 1) {
            *H(f + 506) = 1;
            *B(f + 482) = 0; *B(f + 483) = 8;
            c->r[4] = f; c->r[5] = p + 2; f_08877F44(c);
        } else {
            int16_t g = S16(f + 366);
            if (g < 48) read = 1;
            else if (ism != 0) {
                if (g != 0x90) read = 1;
                else *B(f + 194) = 2;
            }
            if (!read) {
                c->r[4] = f; f_0887B504(c);
                if (c->r[2] == 0) read = 1;
                else {
                    if (*B(f + 4) == 1 && *B(f + 5) == 1 && *B(f + 6) == 2 && *B(f + 7) == 0) {
                        c->r[19] = 2;
                        c->r[4] = f; f_0886B468(c);
                        *B(c->r[2] + 356) = 2;
                    }
                    special_start(f, 5, 0);
                    *B(f + 696) = 0; *B(f + 123) = 0;
                    c->r[4] = f; f_0886DC3C(c);
                    RETURN(p + 2);
                }
            }
        }
    } else {
        if (S8(f + 386) != 1 || S16(f + 366) < 72 || S8(f + 251) != 0) read = 1;
        else {
            c->r[4] = f; f_0887B504(c);
            if (c->r[2] == 0) read = 1;
            else {
                special_start(f, 5, 1);
                *B(f + 693) = 0; *B(f + 696) = 0; *B(f + 123) = 0;
                *B(f + 236) = (uint8_t)S8(f + 490);
                int32_t k = S8(f + 491);
                *B(f + 492) = 0;
                if (k >= 3) { *B(f + 270) = 1; }
                else {
                    if (k == 2)      { *B(f + 548) = 4; *H(f + 662) = 0x51; *H(f + 664) = 0x1B; }
                    else if (k == 1) { *B(f + 548) = 0; *H(f + 662) = 0x30; *H(f + 664) = 0x10; }
                    else if (k == 0) { *B(f + 548) = 0; *H(f + 662) = 0xF;  *H(f + 664) = 5; }
                    *B(f + 270) = 1;
                }
                RETURN(p + 2);
            }
        }
    }
    if (read) { c->r[4] = f; c->r[5] = p + 2; f_0887A10C(c); }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* ---------------------------------------------------------------- the network session */

/* (record, value): when NET_STATE +48 is set, every member's +0xD4 count in state 1 moved to 2 with
   its +2 / +4 / +6 cleared (and NET_STATE +30 / +33 / +42); then +36 = value, +33 / +37 / +30 cleared
   and +24 = the record's +64. */
SFA_FUNC(088E348C, net_members_advance) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    int32_t v = (int8_t)ARG(1);
    c->r[29] -= 0x30;
    c->r[19] = NET_STATE; c->r[20] = (uint32_t)v; c->r[16] = o;
    if (S8(NET_STATE + 48) != 0) {
        f_088DD3DC(c);
        gaddr local = c->r[29] + 16;
        *W(local) = c->r[2];
        c->r[18] = local; c->r[17] = 1;
        for (;;) {
            c->r[4] = local; f_088DD418(c);
            if (c->r[2] == 0) break;
            gaddr m = c->r[2] + 0xD4;
            if (*H(m) != 1) continue;
            *H(m) = 2; *H(m + 2) = 0; *H(m + 6) = 0; *H(m + 4) = 0;
            *H(NET_STATE + 30) = 0; *B(NET_STATE + 33) = 0; *B(NET_STATE + 42) = 0;
        }
        *B(NET_STATE + 36) = (uint8_t)v;
        *B(NET_STATE + 33) = 0; *B(NET_STATE + 37) = 0;
        *H(NET_STATE + 30) = 0;
        *H(NET_STATE + 24) = (uint16_t)S16(o + 64);
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
}

/* (): the network thread's loop: matching ended; while +190, ad hoc closed (+20 = 3; result
   0x80410B08 stops it) and waited on (1 s steps) for flags; then, while the ad hoc control state is
   not 0, +190 set and again. +20 cleared; the last result returned. */
SFA_FUNC(088DE924, net_thread_loop) {
    gaddr t = NET_THREAD;
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21];
    c->r[29] -= 0x30;
    gaddr local = c->r[29] + 16;
    c->r[20] = t; c->r[17] = 0xF4240; c->r[18] = 0x80410B08u; c->r[19] = 3; c->r[21] = 0; c->r[16] = local;
    uint32_t res = 0;
    for (;;) {
        c->r[4] = t; f_088DF0B8(c);
        if (*B(t + 190) != 0) {
            *W(t + 20) = 3;
            c->r[4] = t; f_088DF400(c);
            res = c->r[2]; c->r[21] = res;
            if ((int32_t)res < 0 && res == 0x80410B08u) { *B(t + 190) = 0; break; }
            *W(t + 20) = 3;
            for (;;) {
                c->r[4] = t; f_088DF46C(c);
                res = c->r[2]; c->r[21] = res;
                if (res != 0) break;
                os_delay_thread(c, 0xF4240);
            }
        }
        os_net_adhocctl_get_state(c, local);
        if (*W(local) == 0) { *B(t + 190) = 0; break; }
        if (*B(t + 190) == 0) *B(t + 190) = 1;
    }
    *W(t + 20) = 0;
    RETURN(res);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21;
}
