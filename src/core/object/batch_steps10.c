/* Small steps (0.139.0): the last 4 ready functions of 9 - 40 instructions and 26 of 41 - 60. Objects
   queued by layer and following a fighter's picture, slides and timers, effects made at a fighter's
   side, a fighter's targets by layout, the check that lets a fighter start a move, the network
   session's lists taken and cleared under its lock, and the heap's freed blocks unlinked. Offsets stay
   numbers until something shows what they are. What the original leaves in a0 - t2 at a call is set as
   the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define GAME      0x08E75910u
#define PADS      0x08E75BA0u   /* pad records, 44 bytes apart: +12 pressed */
#define BATTLE    0x092E1228u
#define CAM_0     0x092E8800u   /* camera layer 0: +18 scroll */
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define NET_SEMA  0x08A3EB88u   /* u32: the semaphore "network_sema", once made */
#define NET_LIST  0x095236D4u   /* a list of 12 bytes (first, last, count) */
#define HEAP_BASE 0x08B02EA0u   /* u32: the heap's first block */
#define HEAP_SEMA 0x08B02EA4u
#define HEAP_FREE 0x1AE49CF0u   /* a block's mark once unmarked */
#define POOL_32_FREE 0x095176F3u
#define THREE     0x08B02690u   /* three objects' addresses, lowest first */

void f_088DD84C(Cpu *c);   /* record_release */
void f_088E06D0(Cpu *c);   /* list_unmark_all */
void f_088E0698(Cpu *c);   /* list_take_first */
void f_088DC2C4(Cpu *c);   /* unmark_under_lock */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_088215C4(Cpu *c);   /* object_draw */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_088F16D4(Cpu *c);   /* script_by_health */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_08821368(Cpu *c);   /* game_random */
void f_0884F8AC(Cpu *c);   /* step_by_type_then_draw */
void f_0884FB5C(Cpu *c);   /* animation_by_type_and_direction */
void f_0884FC04(Cpu *c);   /* random_82_by_type */
void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08822F54(Cpu *c);   /* object_make_42 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_0887DF88(Cpu *c);   /* object_set_animation_n */
void f_088219B0(Cpu *c);   /* object_animate */
void f_088594D0(Cpu *c);   /* fighter_pick_target */
void f_08880A04(Cpu *c);   /* owner_free_560 */
void f_08880A30(Cpu *c);   /* place_at_owner_point */
void f_08880B1C(Cpu *c);   /* random_animation */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_0889DB8C(Cpu *c);   /* place_kind */
void f_0886EB48(Cpu *c);   /* fighter_unpack_lists */

/* The network semaphore taken / given back, when it has been made. */
static void net_lock(Cpu *c)   { if (*W(NET_SEMA) != 0) os_wait_sema(c, (int32_t)*W(NET_SEMA), 1, 0); else c->r[5] = 1; }
static void net_unlock(Cpu *c) { if (*W(NET_SEMA) != 0) os_signal_sema(c, (int32_t)*W(NET_SEMA), 1); }

/* The first item of `list` taken under the network lock: its +12 into *out (and +4 into *out2 when
   given), the item unmarked, the list's count left. 0x81001000 for an empty list. */
static void net_take(Cpu *c, gaddr list, gaddr out, gaddr out2) {
    net_lock(c);
    c->r[4] = list; f_088E0698(c);
    gaddr item = c->r[2];
    net_unlock(c);
    if (item == 0) { RETURN(0x81001000u); return; }
    *W(out) = *W(item + 12);
    if (out2 != 0) *W(out2) = *W(item + 4);
    c->r[4] = item; f_088DC2C4(c);
    RETURN(*W(list + 8));
}

/* ---------------------------------------------------------------- the network session's lists */

/* (record, out): the record released; its +0 into *out, or into 0x08B02EA8 when out is 0. Returns it. */
SFA_FUNC(088DD8AC, record_release_next) {
    gaddr r = ARG(0), out = ARG(1), s16 = c->r[16];
    uint32_t next = 0;
    c->r[29] -= 0x20;
    if (r != 0) {
        next = *W(r);
        if (out == 0) { c->r[5] = 0x08B00000u; *W(0x08B02EA8u) = next; }
        else *W(out) = next;
        c->r[16] = next;
        f_088DD84C(c);
    }
    RETURN(next);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the list 0x095236D4 emptied under the network lock. */
SFA_FUNC(088E12FC, net_list_clear) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = 0x08A40000u; c->r[17] = 0x095236C0u;
    net_lock(c);
    c->r[4] = NET_LIST; f_088E06D0(c);
    net_unlock(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (record): under the network lock, its data at +12 and +8 unmarked and cleared, its lists at +0x1C
   and +0x10 emptied. */
SFA_FUNC(088E0B04, net_record_clear) {
    gaddr r = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = r;
    if (r != 0) {
        c->r[17] = 0x08A40000u;
        net_lock(c);
        if (*W(r + 12) != 0) { c->r[4] = *W(r + 12); f_088DC2C4(c); *W(r + 12) = 0; }
        c->r[4] = r + 0x1C; f_088E06D0(c);
        if (*W(r + 8) != 0) { c->r[4] = *W(r + 8); f_088DC2C4(c); *W(r + 8) = 0; }
        c->r[4] = r + 0x10; f_088E06D0(c);
        net_unlock(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (_, record): under the network lock, its lists at +0 and +12 emptied, its data at +28 and +24
   unmarked and cleared; then +40 cleared. */
SFA_FUNC(088E13AC, net_record_clear_lists) {
    gaddr r = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = 0x08A40000u; c->r[16] = r;
    net_lock(c);
    c->r[4] = r; f_088E06D0(c);
    c->r[4] = r + 0xC; f_088E06D0(c);
    if (*W(r + 28) != 0) { c->r[4] = *W(r + 28); f_088DC2C4(c); *W(r + 28) = 0; }
    if (*W(r + 24) != 0) { c->r[4] = *W(r + 24); f_088DC2C4(c); *W(r + 24) = 0; }
    net_unlock(c);
    *B(r + 40) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (out): the first of the list 0x095236D4 taken (see net_take). */
SFA_FUNC(088E09AC, net_list_take) {
    gaddr out = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = out; c->r[18] = NET_LIST; c->r[17] = 0x08A40000u;
    net_take(c, NET_LIST, out, 0);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (record, out, out2): the first of the record's list at +0x1C taken (see net_take). */
SFA_FUNC(088E11C0, net_record_take) {
    gaddr list = ARG(0) + 0x1C, out = ARG(1), out2 = ARG(2);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    c->r[19] = list; c->r[17] = out; c->r[16] = out2; c->r[18] = 0x08A40000u;
    net_take(c, list, out, out2);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (): under the heap's lock, the blocks marked free (0x1AE49CF0) unlinked from the chain of blocks
   (+4 the next) that starts at the heap's first and stays inside its 64 KB. */
SFA_FUNC(088DC0E0, heap_unlink_freed) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    gaddr prev = *W(HEAP_BASE), cur = prev;
    os_wait_sema(c, (int32_t)*W(HEAP_SEMA), 1, 0);
    gaddr base = *W(HEAP_BASE), end = base + 0x10000;
    while (cur >= base && cur < end) {
        uint32_t mark = *W(cur), next = *W(cur + 4);
        if (mark == HEAP_FREE) { *W(prev + 4) = next; cur = *W(prev + 4); }
        else { prev = next; cur = next; }
    }
    os_signal_sema(c, (int32_t)*W(HEAP_SEMA), 1);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (row): a network menu row's start: +128 counts on, its text record (+132) from 0x08A3F650 by +92,
   colours and flags set, the twelve bytes +36 - +51 (but +40, +41, +46, +47) at 0xFF. */
SFA_FUNC(088EB6E4, net_menu_row_start) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 54) = 0x12; *B(o + 88) = 2; *B(o + 55) = 7; *B(o + 56) = 0;
    *B(o + 137) = 0; *B(o + 136) = 0xFF;
    *W(o + 132) = 0x08A3F650u + *B(o + 92) * 12;
    *B(o + 93) = 0; *B(o + 3) = 0; *B(o + 89) = 1; *B(o + 138) = 1;
    static const uint8_t ff[] = { 51, 50, 49, 48, 45, 44, 43, 42, 39, 38, 37, 36 };
    for (unsigned i = 0; i < sizeof ff; i++) *B(o + ff[i]) = 0xFF;
    *W(o + 4) = 0; *W(o + 8) = 0;
    *B(o + 129) = 1;
}

/* ---------------------------------------------------------------- objects */

/* (object): queued in its layer's two lists (+9 the layer, 64 bytes a layer at 0x092E28A0 and
   0x092E2928, counts at 0x092E2898 and 0x092E2920); not while BATTLE +36 hides the fight's objects
   unless the object's +72 is set. */
SFA_FUNC(088218C4, object_queue_by_layer) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 36) != 0 && S8(o + 72) == 0) return;
    uint32_t n = *B(o + 9), i = *B(0x092E2898u + n);
    *W(0x092E28A0u + n * 64 + i * 4) = o;
    *W(0x092E2928u + n * 64 + i * 4) = o;
    *B(0x092E2898u + n) = (uint8_t)(i + 1);
    *B(0x092E2920u + n) = (uint8_t)(i + 1);
}

/* (fighter): unless BATTLE +37 or +78: the first time, its script by health and its next frame with
   +82 = 2; after, +82 counts down and at 0 it goes to routine 1. Drawn if on screen. */
SFA_FUNC(088F1528, script_then_countdown) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        if (*B(o + 6) == 0) {
            *B(o + 6) = 1;
            c->r[4] = o; f_088F16D4(c);
            *H(o + 82) = 2;
            c->r[4] = o; f_088768F0(c);
        } else {
            int16_t n = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)n;
            if (n == 0) { *B(o) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        }
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): 3 when the pool of 32 has 3 free, else 0; three made (kind 3) and their addresses kept
   lowest first at 0x08B02690. */
SFA_FUNC(0887CB3C, make_three_sorted) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    if (S8(POOL_32_FREE) < 3) { RETURN(0); }
    else {
        c->r[4] = 3; c->r[5] = 0; f_08822EAC(c); *W(THREE) = c->r[2];
        c->r[4] = 3; c->r[5] = 0; f_08822EAC(c); *W(THREE + 4) = c->r[2];
        c->r[4] = 3; c->r[5] = 0; f_08822EAC(c); *W(THREE + 8) = c->r[2];
        uint32_t a = *W(THREE), b = *W(THREE + 4), n = c->r[2];
        if (b < a) { *W(THREE) = b; *W(THREE + 4) = a; uint32_t t = a; a = b; b = t; }
        if (n < a) { *W(THREE) = n; *W(THREE + 8) = a; n = a; }
        if (n < b) { *W(THREE + 4) = n; *W(THREE + 8) = b; }
        c->r[16] = 0x08B00000u; c->r[17] = 0x08B00000u;
        RETURN(1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +15 and +4 to 1, its place (+16 / +20) copied to +0x70, +0x80 = +78 * 16, a pair of
   words by +3 from 0x08A15458 into +0x78 and +96, +0x7C a random 0 or 0x10000; then its +82, its
   animation and its step by type. */
SFA_FUNC(0884F7FC, start_by_type_table) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = o + 0x60;
    *B(o + 15) = 1; *B(o + 4) = 1;
    *W(o + 0x70) = *W(o + 16);
    *W(o + 0x74) = *W(o + 20);
    *H(o + 0x82) = 0;
    *H(o + 0x80) = (uint16_t)(S8(o + 78) << 4);
    *B(o + 12) = 0;
    gaddr t = 0x08A15458u + *B(o + 3) * 8;
    *W(o + 0x78) = *W(t);
    *W(o + 96) = *W(t + 4);
    f_08821368(c);
    *W(o + 0x7C) = (c->r[2] & 1) << 16;
    c->r[4] = o; f_0884FC04(c);
    c->r[4] = o; f_0884FB5C(c);
    c->r[4] = o; f_0884F8AC(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object, record): in a network battle, unless record +52: a clock at +141 / +142 / +143 (minutes,
   seconds, frames of 60) counting up to 99 minutes. */
SFA_FUNC(08833068, network_clock) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[17] = r; c->r[16] = o;
    f_088D5E44(c);
    if (c->r[2] != 0 && S8(r + 52) == 0 && S8(o + 141) != 99) {
        *B(o + 143) = (uint8_t)(S8(o + 143) + 1);
        if (S8(o + 143) >= 60) {
            *B(o + 143) = 0;
            *B(o + 142) = (uint8_t)(S8(o + 142) + 1);
            if (S8(o + 142) >= 60) {
                *B(o + 141) = (uint8_t)(S8(o + 141) + 1);
                *B(o + 142) = 0;
                if (S8(o + 141) >= 100) { *B(o + 141) = 99; *B(o + 142) = 0; *B(o + 143) = 0; }
            }
        }
    }
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): while +73 is set, every 16th frame (8th unless +73 is 1) one object of kind 6 / type +3 +
   0x12 made with this one's +78, its +73 picked at random from 0x08A244FE (+8 unless +73 is 1). */
SFA_FUNC(0887EBA4, spawn_by_73) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    int32_t k = S8(o + 73);
    c->r[19] = (uint32_t)k; c->r[16] = o;
    if (k != 0) {
        c->r[18] = 1;
        uint32_t mask = k == 1 ? 0xF : 7;
        if ((*B(GAME + 32) & mask) == 0) {
            c->r[4] = 6; c->r[5] = (*B(o + 3) + 0x12) & 0xFF;
            f_08822EAC(c);
            gaddr n = c->r[2];
            c->r[17] = n;
            if (n != 0) {
                *B(n + 78) = (uint8_t)S8(o + 78);
                f_08821368(c);
                uint32_t i = c->r[2] & 7;
                if (k != 1) i += 8;
                *B(n + 73) = *B(0x08A244FEu + i);
            }
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (fighter, kind): two objects of kind 0xA (three when kind is 4), types 0, 2, 4, at the fighter's
   side: +14 and +24 copied, +78 the kind, +82 its character, +88 the fighter. */
SFA_FUNC(08880B84, make_side_effects) {
    gaddr f = ARG(0); uint32_t kind = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    c->r[29] -= 0x30;
    c->r[17] = f; c->r[16] = kind;
    int n = kind == 4 ? 3 : 2;
    c->r[20] = (uint32_t)n;
    for (int i = 0; i < n; i++) {
        c->r[19] = (uint32_t)i; c->r[18] = (uint32_t)i * 2;
        c->r[4] = 0xA; c->r[5] = ((uint32_t)i * 2) & 0xFF;
        f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 14) = *B(f + 14);
            *H(e + 24) = *H(f + 24);
            *B(e + 78) = (uint8_t)kind;
            *H(e + 82) = (uint16_t)(int16_t)S8(f + 338);
            *W(e + 88) = f;
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
}

/* (object): its animation (0x08A15F18) started once camera layer 0 scrolls into 1281 - 1535 (state +7
   0, then 1), and again at 14337 - 14591 (state back to 0). Drawn if on screen when it starts. */
SFA_FUNC(08853070, animate_at_camera_x) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    int32_t x = S16(CAM_0 + 18);
    int go = 0;
    if (st == 0) { if (x >= 1281 && x < 1536) { *B(o + 5) = 1; *B(o + 7) = 1; go = 1; } }
    else if (st == 1) { if (x >= 14337 && x < 14592) { *B(o + 5) = 1; *B(o + 7) = 0; go = 1; } }
    if (go) {
        c->r[4] = o; c->r[5] = *W(0x08A15F18u); f_0882196C(c);
        c->r[4] = o; f_08821424(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the target picked by the fight's layout (BATTLE +56): 1 fighter 1 against 3 and 4, 2 fighter 2
   against 3 and 4, 3 fighter 3 against 1 and 2. */
SFA_FUNC(08859418, targets_by_layout) {
    int32_t l = S16(BATTLE + 56);
    gaddr a, b, d;
    if (l == 1)      { a = FIGHTER0; b = FIGHTER2; d = FIGHTER3; }
    else if (l == 2) { a = FIGHTER1; b = FIGHTER2; d = FIGHTER3; }
    else if (l == 3) { a = FIGHTER2; b = FIGHTER0; d = FIGHTER1; }
    else return;
    c->r[29] -= 0x20;
    c->r[4] = a; c->r[5] = b; c->r[6] = d;
    f_088594D0(c);
    c->r[29] += 0x20;
}

/* An object following a fighter's picture: if the fighter's character (+338) is the object's (+3),
   the picture number at its frame (+296 +10) chosen from `table` and the place and facing copied;
   else +4 counts on. `nudge` moves it 5 left when the fighter faces right (and leaves +11 at 0). */
static void follow_picture(Cpu *c, gaddr o, gaddr f, gaddr table, int nudge) {
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    if ((uint32_t)(int32_t)S8(f + 338) != *B(o + 3)) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    } else {
        uint8_t pic = *B(*W(f + 296) + 10);
        int8_t n = (int8_t)pic;
        if (n == 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(f + 18);
            *H(o + 22) = (uint16_t)S16(f + 22);
            if (nudge) {
                *B(o + 11) = 0;
                c->r[5] = *B(f + 11);
                if (c->r[5] != 0) *H(o + 18) = (uint16_t)(S16(o + 18) - 5);
                c->r[5] = 1;
            } else {
                c->r[5] = *B(f + 11);
                *B(o + 11) = (uint8_t)c->r[5];
            }
            *B(o + 1) = 1;
            c->r[4] = o;
            if (n == S8(o + 78)) f_088219B0(c);
            else {
                *B(o + 78) = pic;
                c->r[5] = *W(table + (uint32_t)(int32_t)n * 4);
                f_0882196C(c);
            }
        }
    }
    c->r[29] += 0x20;
}

/* (object, fighter): follow_picture with the pictures of 0x08A51888. */
SFA_FUNC(08908CD0, follow_picture_51888) { follow_picture(c, ARG(0), ARG(1), 0x08A51888u, 0); }
/* (object, fighter): follow_picture with the pictures of 0x08A43F8C. */
SFA_FUNC(088F5B9C, follow_picture_43f8c) { follow_picture(c, ARG(0), ARG(1), 0x08A43F8Cu, 0); }
/* (object, fighter): follow_picture with the pictures of 0x08A6C0F8. */
SFA_FUNC(08933420, follow_picture_6c0f8) { follow_picture(c, ARG(0), ARG(1), 0x08A6C0F8u, 0); }
/* (object, fighter): follow_picture with the pictures of 0x08A45700, 5 left when facing right. */
SFA_FUNC(088FC514, follow_picture_45700) { follow_picture(c, ARG(0), ARG(1), 0x08A45700u, 1); }

/* (object): in three parts (+129): moves by +104 on +6 for +98 frames, then to 0xC0; waits for
   BATTLE +18 to be 0 and sets 11 frames; then moves by +108 on +10 for them and sets +128 = 2. +93 set
   each frame. */
SFA_FUNC(08887E00, slide_wait_slide) {
    gaddr o = ARG(0);
    uint32_t s = *B(o + 129);
    if (s == 0) {
        *H(o + 6) = (uint16_t)(S16(o + 6) + *W(o + 104));
        uint16_t t = (uint16_t)(*H(o + 98) - 1);
        *H(o + 98) = t;
        if (t & 0x8000) { *B(o + 129) = 1; *H(o + 6) = 0xC0; }
    } else if (s == 1) {
        if (S16(BATTLE + 18) == 0) { *B(o + 129) = 2; *H(o + 98) = 0xB; }
    } else {
        *H(o + 10) = (uint16_t)(S16(o + 10) + *W(o + 108));
        uint16_t t = (uint16_t)(*H(o + 98) - 1);
        *H(o + 98) = t;
        if (t & 0x8000) *B(o + 128) = 2;
    }
    *B(o + 93) = 1;
}

/* (object): an effect on its owner: first a random animation and placed at the owner's point; then
   placed and animated until the owner frees it (animation 0x33 + 2 * +78 next); then animated while
   +75 is not below 0, else ended (+4 = 2). Drawn each frame. */
SFA_FUNC(088807D8, effect_on_owner) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t s = *B(o + 5);
    if (s == 0) {
        *B(o + 5) = 1;
        c->r[4] = o; c->r[5] = (uint32_t)(int32_t)S8(o + 78); f_08880B1C(c);
        c->r[4] = o; f_08880A30(c);
    } else if (s == 1) {
        c->r[4] = o; f_08880A04(c);
        if (c->r[2] != 0) {
            *B(o + 5) = 2;
            c->r[4] = o; c->r[5] = (uint32_t)(S8(o + 78) * 2 + 0x33) & 0xFFFF;
            f_0887DF88(c);
        }
        c->r[4] = o; f_08880A30(c);
        c->r[4] = o; f_088219B0(c);
    } else {
        if (S8(o + 75) >= 0) { c->r[4] = o; f_088219B0(c); }
        else *B(o + 4) = 2;
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): outside a network battle, 1 when a pad its +53 names (bits 0 - 2) has pressed one of the
   buttons 0x83F0, else 0. */
SFA_FUNC(0882B9BC, named_pad_pressed) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = o; c->r[17] = 0;
    f_088D5E44(c);
    if (c->r[2] != 0) RETURN(0);
    else {
        uint32_t m = (uint32_t)(int32_t)S8(o + 53), hit = 0;
        for (int i = 0; i < 3; i++)
            if ((m & (1u << i)) && (*W(PADS + 12 + i * 44) & 0x83F0)) hit = 1;
        RETURN(hit);
    }
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, take): 1 when it may start a move: in routine 1 step 0 (and, with +251, neither +262 nor
   BATTLE +13); not in part 8 or 9; part 7 only with `take` and +792 bits 0x600004 (or with +251) and
   then +135 clear;
   part 5 with +135 clear and one of +792 bits 0x500002, +643 (with `take`), +542 or +80; and +702
   clear. Else 0. */
SFA_FUNC(0886C164, fighter_may_start) {
    gaddr f = ARG(0); uint32_t take = ARG(1);
    int ok = 1;
    uint32_t part = *B(f + 6);
    if (*B(f + 4) != 1 || *B(f + 5) != 0) ok = 0;
    else if (S8(f + 251) != 0 && (S8(f + 262) | S8(BATTLE + 13)) != 0) ok = 0;
    else if (part == 8 || part == 9) ok = 0;
    else if (part == 7) {
        int held = take != 0 && (*W(f + 792) & 0x600004u) != 0;
        if (S8(f + 251) == 0 && !held) ok = 0;          /* with +251 set, part 7 passes without them */
        else if (held && S8(f + 135) != 0) ok = 0;
    } else if (part == 5) {
        if (S8(f + 135) != 0) ok = 0;
        else if (!(*W(f + 792) & 0x500002u) && !(take != 0 && S8(f + 643) != 0) && S8(f + 542) == 0
                 && S8(f + 80) == 0) ok = 0;
    }
    if (ok && S8(f + 702) != 0) ok = 0;
    RETURN(ok);
}

/* (fighter, script): script command: +490 = the next s16; when the opponent's +73 is 1 and its height
   (+22) is above that plus its +132, or it moves up (+100 >= 0), routine 1 with +483 = 4; else the
   script read on. */
SFA_FUNC(0887773C, script_wait_opponent_height) {
    gaddr f = ARG(0), p = ARG(1), opp = *W(f + 92);
    int16_t v = S16(p);
    c->r[29] -= 0x20;
    *H(f + 490) = (uint16_t)v;
    int wait = 0;
    if (S8(opp + 73) == 1) {
        int16_t y = (int16_t)(v + S16(opp + 132));
        if (y < S16(opp + 22)) wait = 1;
        else if ((int32_t)*W(opp + 100) >= 0) wait = 1;
    }
    if (wait) {
        *B(f + 482) = 0; *B(f + 483) = 4;
        *B(f + 4) = 1; *B(f + 7) = 0; *B(f + 6) = 0; *B(f + 5) = 0;
        RETURN(p + 2);
    } else {
        c->r[5] = p + 2; f_0887A10C(c);
    }
    c->r[29] += 0x20;
}

/* (fighter, x): its count at +426 shown as text at (x, 0): the text 0x089FEF64 for 1, else two
   digits (a space for tens of 0) in the buffer *0x08A25A48. */
SFA_FUNC(0888722C, show_count_426) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    int32_t x = (int16_t)ARG(1);
    c->r[29] -= 0x30;
    int32_t v = S16(f + 426);
    c->r[6] = (uint32_t)x;
    if (v != 0) {
        c->r[17] = 1; c->r[16] = 7;
        if (v == 1) c->r[4] = 0x089FEF64u;
        else {
            gaddr buf = *W(0x08A25A48u);
            DIV(c, (uint32_t)v, 10);
            c->r[9] = c->lo;
            *B(buf) = c->lo == 0 ? 0x20 : (uint8_t)(c->lo + 0x30);
            DIV(c, (uint32_t)v, 10);
            c->r[10] = c->hi + 0x30;
            *B(buf + 1) = (uint8_t)c->r[10];
            c->r[4] = buf;
        }
        c->r[5] = 1; c->r[7] = 0; c->r[8] = 5;
        f_0880A6CC(c);
        *B(c->r[2] + 3) = 7;
        *B(c->r[2] + 12) = 1;
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): thrown from its owner (+88): placed (kind 1, -408, 120), +86 = 13, +164 = 1, the owner's
   +24, speeds 0x320000 / -0x20000, animation 0x08A2AB88; for the characters 0x08A2ABC8 marks, 0x4B0
   further right and both speeds turned round. */
SFA_FUNC(0889D2AC, thrown_from_owner) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    gaddr owner = *W(o + 88);
    c->r[16] = o; c->r[17] = owner; c->r[18] = 1;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; c->r[5] = 1; c->r[6] = 0xFFFFFE68u; c->r[7] = 0x78; f_0889DB8C(c);
    *W(o + 148) = 0;
    *H(o + 86) = 0xD;
    *W(o + 164) = 1;
    *H(o + 24) = *H(owner + 24);
    *W(o + 96) = 0x320000u;
    *W(o + 100) = 0xFFFE0000u;
    c->r[4] = o; c->r[5] = 0x08A2AB88u; f_0882196C(c);
    if (*B(0x08A2ABC8u + (uint32_t)(int32_t)S8(owner + 338)) != 0) {
        *H(o + 18) = (uint16_t)(S16(o + 18) + 0x4B0);
        *W(o + 96) = 0u - *W(o + 96);
        *W(o + 100) = 0u - *W(o + 100);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (fighter): +171 cleared, +12 = -1, its table (+416) filled with 39 words from 0x08A53774, lists
   unpacked (5, 0x08A53810), +164 = 1; an object of kind 5 made (+40) that follows it (+88), with its
   place, facing, +12 - +14 and character. */
SFA_FUNC(08905614, fighter_start_table_5) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    gaddr dst = *W(f + 416);
    for (uint32_t i = 0; i < 39; i++) *W(dst + i * 4) = *W(0x08A53774u + i * 4);
    c->r[4] = f; c->r[5] = 5; c->r[6] = 0x08A53810u; f_0886EB48(c);
    *W(f + 164) = 1;
    c->r[4] = 5; c->r[5] = 0; f_08822F54(c);
    gaddr o = c->r[2];
    if (o != 0) {
        *W(o + 88) = f;
        *W(f + 40) = o;
        *W(o + 16) = *W(f + 16);
        *W(o + 20) = *W(f + 20);
        *B(o + 11) = *B(f + 11);
        *B(o + 14) = *B(f + 14);
        *B(o + 12) = (uint8_t)S8(f + 12);
        *B(o + 13) = *B(f + 13);
        *B(o + 3) = (uint8_t)S8(f + 338);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* One fighter of fighters_lose_one: +0 counts on; unless +255 is 1 or 6, routine 1 and one off its
   health (+112); at 1 or 6, or when the health goes below 0, routine 2. */
static void lose_one(gaddr x) {
    *B(x) = (uint8_t)(*B(x) + 1);
    int8_t k = S8(x + 255);
    if (k != 1 && k != 6) {
        *B(x + 4) = 1; *B(x + 5) = 1; *B(x + 6) = 0; *B(x + 7) = 0;
        int16_t h = (int16_t)(S16(x + 112) - 1);
        *H(x + 112) = (uint16_t)h;
        if (h >= 0) return;
    }
    *B(x + 4) = 2; *B(x + 5) = 0; *B(x + 6) = 0; *B(x + 7) = 0;
}

/* (a, b): lose_one for both fighters. */
SFA_FUNC(0887456C, fighters_lose_one) {
    lose_one(ARG(0));
    lose_one(ARG(1));
}
