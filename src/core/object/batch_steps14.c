/* Small steps (0.143.0): the 1 function of 9 - 60 instructions made ready by the batch before and 6 of
   61 - 100. The network thread's outer entry, the heap's allocation, a screen's banner by kind, a
   picture by slot, CPU levels for a versus fight, a round's end by record and 17 objects by layout.
   Offsets stay numbers until something shows what they are. What the original leaves in a0 - t1 at
   a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME       0x08E75910u
#define BATTLE     0x092E1228u
#define FIGHTER0   0x092E3EA0u
#define FIGHTER1   0x092E4338u
#define FIGHTER2   0x092E47D0u
#define FIGHTER3   0x092E4C68u
#define NET_THREAD 0x09523740u
#define HEAP_BASE  0x08B02EA0u   /* u32: the heap's first block */
#define HEAP_SEMA  0x08B02EA4u
#define HEAP_FREE  0x1AE49CF0u   /* a block's mark when free; + 1 in use */
#define BANNER_ON  0x09525E00u   /* u8: a banner is shown; its object at 0x09525E04 */
#define ENTRIES    0x09358528u   /* the saved entries, 0x354 apart: +0 in use, +4 total level */

void f_088DEA18(Cpu *c);   /* net_thread_entry */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_088233BC(Cpu *c);   /* object_free_list_3 */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088D0BA8(Cpu *c);   /* copy_record_8 */
void f_08821368(Cpu *c);   /* game_random */
void f_0882655C(Cpu *c);   /* fighters_reset_424 */
void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_08822EAC(Cpu *c);   /* object_make_32 */

/* ---------------------------------------------------------------- the network thread and the heap */

/* (): the network thread's outer entry: +188 set while net_thread_entry runs, +4 cleared, then the
   thread's info freed and the thread ended. */
SFA_FUNC(088DEA78, net_thread_main) {
    gaddr t = NET_THREAD, s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = t;
    *B(t + 188) = 1;
    f_088DEA18(c);
    *W(t + 4) = 0;
    *B(t + 188) = 0;
    int32_t id = os_get_thread_id(c);
    os_net_free_threadinfo(c, id);
    os_exit_thread(c, 0);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (size): under the heap's lock, the first run of free blocks (marked 0x1AE49CF0, merged as it goes)
   that holds size + 8 (rounded to 4) taken: marked in use, split when more than 8 bytes are left.
   Returns its data (block + 8), or 0. */
SFA_FUNC(088DC18C, heap_alloc) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21];
    c->r[29] -= 0x30;
    uint32_t need = ARG(0) + 8;
    gaddr cur = *W(HEAP_BASE), run = cur, got = 0;
    if (need & 3) need = (need & ~3u) + 4;
    c->r[21] = 0x08B00000u;
    os_wait_sema(c, (int32_t)*W(HEAP_SEMA), 1, 0);
    gaddr base = *W(HEAP_BASE), end = base + 0x10000;
    while (!(cur < base) && cur < end) {
        uint32_t mark = *W(cur), next = *W(cur + 4);
        if (mark != HEAP_FREE) { run = next; cur = next; continue; }
        if (run != cur) { *W(run + 4) = next; cur = run; next = *W(cur + 4); }
        if (next - cur < need) { cur = next; continue; }
        gaddr split = run + need;
        if (split + 8 < next) { *W(split) = HEAP_FREE; *W(split + 4) = *W(cur + 4); }
        else split = next;
        *W(run) = HEAP_FREE + 1;
        *W(run + 4) = split;
        got = run + 8;
        break;
    }
    os_signal_sema(c, (int32_t)*W(HEAP_SEMA), 1);
    RETURN(got);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21;
}

/* ---------------------------------------------------------------- screens and objects */

/* (kind): the banner shown before by 0x09525E00 freed; kind kept at 0x09358C4E; by the table 0x08A01D20
   (kinds below 19) a banner of list 3 (kind 0xD, type kind + 0x10; or type 0x17 with a second one), or
   none; GAME +93 (and +94) cleared for some, and for kinds 19 and up. */
SFA_FUNC(089E0324, banner_by_kind) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    int32_t k = (int16_t)ARG(0);
    c->r[17] = (uint32_t)k; c->r[16] = 0x09520000u;
    if (*B(BANNER_ON) != 0) {
        *B(BANNER_ON) = 0;
        c->r[4] = *W(BANNER_ON + 4); f_088233BC(c);
    }
    *H(0x09358C4Eu) = (uint16_t)k;
    if ((uint32_t)k < 0x13u) {
        uint32_t to = *W(0x08A01D20u + (uint32_t)k * 4);
        uint32_t type = ((uint32_t)k + 0x10) & 0xFF;
        switch (to) {
        case 0x089E0388u:
            *B(BANNER_ON) = 0; *B(GAME + 93) = 0; *B(GAME + 94) = 0;
            break;
        case 0x089E03A0u:
            *B(BANNER_ON) = 1;
            c->r[4] = 0xD; c->r[5] = type; f_08823338(c);
            *W(BANNER_ON + 4) = c->r[2];
            break;
        case 0x089E03C4u:
            *B(BANNER_ON) = 1;
            c->r[4] = 0xD; c->r[5] = 0x17; f_08823338(c);
            *W(BANNER_ON + 4) = c->r[2];
            c->r[4] = 0xD; c->r[5] = type; f_08823338(c);
            break;
        case 0x089E03F8u:
            *B(BANNER_ON) = 1;
            c->r[4] = 0xD; c->r[5] = type; f_08823338(c);
            *W(BANNER_ON + 4) = c->r[2];
            *B(GAME + 93) = 0; *B(GAME + 94) = 0;
            break;
        case 0x089E042Cu:
            *B(GAME + 93) = 0;
            break;
        default: rt_bad_jump(c, to, 0x089E037Cu); return;
        }
    } else {
        *B(GAME + 93) = 0;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object, slot, first, wanted): the owner's (+88) record for `slot` copied (8 halfwords) to the stack;
   the value at +3 - first under 15 picks the picture: with the owner's +3 bit for the slot set, picture
   (+3 - first, 14 from 6) of 0x08A3CC78; else at +3 - first = wanted the one 0x08A058A0 names (slot
   * 20 + wanted * 2), unless that is 15 or more. Drawn unless it stopped there. */
SFA_FUNC(088D0C08, picture_by_slot) {
    gaddr o = ARG(0);
    uint32_t slot = ARG(1) & 0xFF, first = ARG(2) & 0xFF, want = ARG(3) & 0xFF;
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21];
    c->r[29] -= 0x40;
    gaddr own = *W(o + 88), local = c->r[29] + 0x10;
    c->r[16] = o; c->r[19] = slot; c->r[20] = own; c->r[21] = first; c->r[18] = want; c->r[17] = 0x08A058A0u;
    c->r[4] = local; c->r[5] = slot; f_088D0BA8(c);
    uint32_t idx = *B(o + 3) - first;
    int draw = 0;
    if (*H(local + idx * 2) < 15) {
        if ((uint32_t)(int32_t)S8(own + 3) & (1u << (slot & 31))) {
            int32_t i = (int16_t)idx;
            if (i >= 6) i = 0xE;
            c->r[4] = o; c->r[5] = *W(0x08A3CC78u + (uint32_t)i * 4); f_0882196C(c);
            draw = 1;
        } else {
            int32_t pic = (int16_t)*H(0x08A058A0u + slot * 20 + want * 2);
            if (idx != want) draw = 1;
            else if (pic < 15) {
                c->r[4] = o; c->r[5] = *W(0x08A3CC78u + (uint32_t)pic * 4); f_0882196C(c);
                draw = 1;
            }
        }
    }
    if (draw) { c->r[4] = o; f_088215C4(c); }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21;
}

/* (): CPU levels for a versus fight: both at 0x10, the side behind on +374 raised by a random step
   from 0x08A0F9F0; a CPU (+373 below 0) playing a saved entry gets its level from 0x08A0FA00 by the
   entry's +4. */
SFA_FUNC(088366D8, cpu_levels_versus) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[17] = FIGHTER0; c->r[16] = FIGHTER1;
    *B(FIGHTER0 + 375) = 0x10;
    *B(FIGHTER1 + 375) = 0x10;
    f_08821368(c);
    int32_t step = S8(0x08A0F9F0u + (c->r[2] & 0xF));
    int16_t d = (int16_t)(S8(FIGHTER0 + 374) - S8(FIGHTER1 + 374));
    if (d > 0) *B(FIGHTER1 + 375) = (uint8_t)(S8(FIGHTER1 + 375) + step);
    else if (d < 0) *B(FIGHTER0 + 375) = (uint8_t)(S8(FIGHTER0 + 375) + step);
    gaddr e0 = ENTRIES, e1 = ENTRIES + 0x354;
    if (S8(e0) != 0 && S8(FIGHTER0 + 373) < 0) *B(FIGHTER0 + 375) = *B(0x08A0FA00u + (*B(e0 + 4) & 0x1F)) & 0x1F;
    if (S8(e1) != 0 && S8(FIGHTER1 + 373) < 0) *B(FIGHTER1 + 375) = *B(0x08A0FA00u + (*B(e1 + 4) & 0x1F)) & 0x1F;
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (game, record): a round's end by its record (+112 / +116 the fighters): with +5 clear, fighters
   reset when +79 or +63; with +5, unless +70: reset when the game's +50; with layout 3 (+56) only when
   the first's +337 has bit 2; else both sides' +336 cleared, the second's side bit off the game's +57,
   its +424 cleared, and the first reset (in a network battle or as a human, +336 = 2 instead). */
SFA_FUNC(08826424, round_end_by_record) {
    gaddr g = ARG(0), rec = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    gaddr first = *W(rec + 112), second = *W(rec + 116);
    c->r[18] = first; c->r[17] = g; c->r[16] = rec;
    int reset = 0;
    if (S8(rec + 5) == 0) reset = S8(rec + 79) != 0 || S8(rec + 63) != 0;
    else if (S8(rec + 70) == 0) {
        if (S8(g + 50) != 0) reset = 1;
        else if (S16(rec + 56) == 3) reset = (S8(first + 337) & 2) != 0;
        else {
            *B(FIGHTER0 + 336) = 0;
            *B(FIGHTER1 + 336) = 0;
            *B(g + 57) = (uint8_t)(S8(g + 57) & ((1u << (S8(second + 337) & 31)) ^ 0xFF));
            *H(second + 424) = 0;
            if (S8(first + 373) == 0) *B(first + 336) = 2;
            else {
                f_088D5E44(c);
                if (c->r[2] != 0) *B(first + 336) = 2;
                else reset = 1;
            }
        }
    }
    if (reset) { c->r[4] = g; c->r[5] = rec; f_0882655C(c); }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (): with 17 free in the pool of 32, 17 objects of kind 0x47 made from the triples at 0x08A2B9FF (type,
   which of three fighters by the layout BATTLE +56 for +92, +10); then a list-3 object of kind 0xA. */
SFA_FUNC(088A4B30, objects_17_by_layout) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x30;
    gaddr arr = c->r[29] + 16;
    int32_t l = S16(BATTLE + 56);
    c->r[16] = 0x11;
    if (l == 1)      { *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER2; *W(arr + 8) = FIGHTER3; }
    else if (l == 2) { *W(arr) = FIGHTER1; *W(arr + 4) = FIGHTER2; *W(arr + 8) = FIGHTER3; }
    else if (l == 3) { *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER1; *W(arr + 8) = FIGHTER2; }
    if (!((uint32_t)(int32_t)S8(0x095176F3u) < 0x11u)) {
        gaddr p = 0x08A2B9FFu;
        for (uint32_t n = 0x11; n != 0; n--) {
            c->r[16] = n;
            c->r[4] = 0x47; c->r[5] = *B(p) & 0x7F;
            f_08822EAC(c);
            gaddr o = c->r[2];
            c->r[17] = p + 1;
            *W(o + 92) = *W(arr + *B(p + 1) * 4);
            *B(o + 10) = *B(p + 2);
            p += 3;
        }
        c->r[16] = 0;
    }
    c->r[4] = 0xA; c->r[5] = 0; f_08823338(c);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17;
}
