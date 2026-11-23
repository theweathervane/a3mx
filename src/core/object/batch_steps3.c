/* Small steps (0.131.0), the rest of the profile's ready functions of 9 - 16 instructions: objects moved
   by their speeds and counting down, fighters' script starts and button checks, the network session's
   list, limits and scan, and the address and nickname of this PSP. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "../decomp.h"
#include "../psp_os.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define SESSION   0x09523820u   /* the network session's records (0x09523920 its state) */
#define NET       0x09523980u   /* the network task's state */
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u   /* FIGHTER_1 + 0x498 */
#define POOL_7    0x09513F70u   /* 16 objects of 0xC0 */
#define POOL_7_FREE 0x095176F4u
#define FADE_BY_SLOT 0x092E11A0u
#define SCENE     0x09358BE0u   /* the ending's state (task/ending.c) */

void f_08822FE4(Cpu *c);   /* object_make */
void f_088230F4(Cpu *c);   /* object_clear_for_use */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_0884FEAC(Cpu *c);   /* move_by_96_100 */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_088D5E7C(Cpu *c);   /* net_waits_for_host */
void f_08821368(Cpu *c);   /* game_random */

/* ---------------------------------------------------------------- objects */

/* (object): +100 not yet falling (negative) and +132 + 16 above +22: 1, else 0. */
SFA_FUNC(0895DE28, rising_above_132) {
    gaddr o = ARG(0);
    int32_t v = (int32_t)*W(o + 100);
    if (v < 0) {
        int32_t top = S16(o + 132) + 0x10, y = S16(o + 22);
        c->r[5] = (uint32_t)top;
        c->r[4] = top < y;
        if (top < y) { RETURN(1); return; }
    }
    RETURN(0);
}

/* (object, record): the next step (+4) once the record's +2 reaches the object's +119. */
SFA_FUNC(089DA968, step_when_2_reaches_119) {
    gaddr o = ARG(0), r = ARG(1);
    int32_t limit = *B(o + 119);
    c->r[6] = (uint32_t)limit;
    if (!(S16(r + 2) < limit)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object): its speeds +96 - +108 from whole numbers to the 8-bit fractions moves use. */
SFA_FUNC(089DBF60, speeds_to_fixed) {
    gaddr o = ARG(0);
    for (uint32_t k = 96; k <= 108; k += 4) *W(o + k) <<= 8;
}

/* (object, n): the n-th animation of the ending's list (SCENE +280). */
SFA_FUNC(089DC074, ending_animation) {
    c->r[29] -= 0x20;
    gaddr list = *W(SCENE + 280);
    c->r[6] = list;
    c->r[5] = *W(list + (ARG(1) & 0xFFFF) * 4);
    f_0882196C(c);
    c->r[29] += 0x20;
}

/* (kind, type): an object from pool 7, looked for from its last. */
SFA_FUNC(08822EE4, object_make_7) {
    c->r[29] -= 0x30;
    uint32_t type = ARG(1) & 0xFF, kind = ARG(0) & 0xFF;
    c->r[4] = POOL_7 + 0xB40;
    c->r[5] = kind; c->r[6] = type;
    c->r[7] = 0x10;
    c->r[8] = POOL_7_FREE;
    f_08822FE4(c);
    c->r[29] += 0x30;
}

/* (record): +16 counts down; past 0 the next step (+4), +43 and +72 cleared. */
SFA_FUNC(0882B35C, count_16_then_4) {
    gaddr r = ARG(0);
    int16_t v = (int16_t)(S16(r + 16) - 1);
    *H(r + 16) = (uint16_t)v;
    if (v < 0) {
        *B(r + 43) = 0;
        *H(r + 4) = (uint16_t)(*H(r + 4) + 1);
        *B(r + 72) = 0;
    }
}

/* (record, other): +16 counts down; past 0 the next step (+4), the other's +81 cleared and +16 = 120. */
SFA_FUNC(0882B77C, count_16_then_4_again) {
    gaddr r = ARG(0);
    int16_t v = (int16_t)(S16(r + 16) - 1);
    *H(r + 16) = (uint16_t)v;
    if (v < 0) {
        *H(r + 4) = (uint16_t)(*H(r + 4) + 1);
        *B(ARG(1) + 81) = 0;
        *H(r + 16) = 0x78;
    }
}

/* (from, count, level): the fade of slots from .. from + count - 1 set to level. */
SFA_FUNC(0882DE00, fade_slots) {
    int32_t from = (int32_t)ARG(0), end = (int32_t)(ARG(1) + ARG(0));
    for (int32_t i = from; i < end; i++) *B(FADE_BY_SLOT + (uint32_t)i) = (uint8_t)ARG(2);
}

/* (object): the owner's (+88) count +536 down by one, its +228 cleared at 0; the object then made
   ready for use. Three objects' steps are this. */
static void owner_536_down_then_clear(Cpu *c) {
    gaddr owner = *W(ARG(0) + 88);
    c->r[29] -= 0x20;
    uint8_t n = (uint8_t)(*B(owner + 536) - 1);
    *B(owner + 536) = n;
    if (n == 0) *W(owner + 228) = 0;
    c->r[5] = owner; c->r[6] = n;
    f_088230F4(c);
    c->r[29] += 0x20;
}
SFA_FUNC(088F169C, owner_536_down_088F169C) { owner_536_down_then_clear(c); }
SFA_FUNC(088FC40C, owner_536_down_088FC40C) { owner_536_down_then_clear(c); }
SFA_FUNC(089A55BC, owner_536_down_089A55BC) { owner_536_down_then_clear(c); }

/* (object): +16 / +20 moved by +96 / +100, which grow by +104 / +108. */
SFA_FUNC(08961A6C, move_accelerating) {
    gaddr o = ARG(0);
    uint32_t vx = *W(o + 96), vy = *W(o + 100);
    *W(o + 16) += vx;
    *W(o + 96) = vx + *W(o + 104);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
}

/* (object): moved and falling as 0x08961A6C, without +96's growth; 1 once +22 is down to +132. */
SFA_FUNC(088EFBF4, fall_to_132) {
    gaddr o = ARG(0);
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 16) += *W(o + 96);
    *W(o + 100) = vy + *W(o + 108);
    RETURN(!(S16(o + 132) < S16(o + 22)));
}

/* (object): +5 = 0 from 768 down (+18); moved by its speeds, then animated. */
SFA_FUNC(0884FE6C, move_then_animate) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (!(S16(o + 18) < 768)) *B(o + 5) = 0;
    c->r[4] = o; f_0884FEAC(c);
    c->r[4] = o; f_088219B0(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): when +75 is negative the next part (+5), +82 = 1, +78 cleared; then animated. */
SFA_FUNC(089619A0, part_when_75_then_animate) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (S8(o + 75) < 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 1;
        *B(o + 78) = 0;
    }
    f_088219B0(c);
    c->r[29] += 0x20;
}

/* (object): +692 one of the sixteen half-words at 0x08A53B7C, at random. */
SFA_FUNC(089087C0, random_692) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_08821368(c);
    c->r[5] = 0x08A53B7Cu;
    c->r[4] = (uint32_t)(int32_t)S16(0x08A53B7Cu + (c->r[2] & 0xF) * 2);
    *B(o + 692) = (uint8_t)c->r[4];
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, script): its script started at the script's word (+506), routine 1 / 0 / 2 / 0, +482 = 1,
   +483 = 3; returns the script after the word. */
SFA_FUNC(08878148, fighter_script_begin_3) {
    gaddr f = ARG(0), p = ARG(1);
    *H(f + 506) = (uint16_t)S16(p);
    *B(f + 482) = 1;
    *B(f + 483) = 3;
    *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
    *B(f + 78) = 0;
    *B(f + 497) = 0;
    c->r[5] = 1; c->r[6] = 2;
    RETURN(p + 2);
}

/* The same with +483 = 4, and +78 / +497 set. */
SFA_FUNC(08878184, fighter_script_begin_4) {
    gaddr f = ARG(0), p = ARG(1);
    *H(f + 506) = (uint16_t)S16(p);
    *B(f + 482) = 1;
    *B(f + 483) = 4;
    *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
    *B(f + 78) = 1;
    *B(f + 497) = 1;
    c->r[5] = 1; c->r[6] = 2;
    RETURN(p + 2);
}

/* (fighter): 1 with bit 14 pressed (+760), or with bit 4 held (+752) and bit 15 pressed or released
   (+760 / +762); else 0. */
SFA_FUNC(088688EC, fighter_pressed_4000_or_8000) {
    gaddr f = ARG(0);
    uint32_t pressed = *H(f + 760);
    if (pressed & 0x4000) { RETURN(1); return; }
    RETURN(0);
    if ((*H(f + 752) & 0x10) == 0) return;
    if ((pressed | *H(f + 762)) & 0x8000) RETURN(1);
}

/* (fighter or record): both fighters' +424, +412, +336 cleared and +360 = 1; its own +57 cleared and
   +69 = 1. */
SFA_FUNC(0882655C, fighters_reset_424) {
    gaddr r = ARG(0);
    *B(r + 57) = 0;
    *B(r + 69) = 1;
    gaddr f[2] = { FIGHTER_1, FIGHTER_2 };
    for (int i = 0; i < 2; i++) {
        *H(f[i] + 424) = 0;
        *B(f[i] + 412) = 0;
        *B(f[i] + 336) = 0;
        *B(f[i] + 360) = 1;
    }
    c->r[4] = FIGHTER_1; c->r[5] = 1; c->r[6] = FIGHTER_2;
}

/* (): how many fighters this layout (BATTLE +56) has as character 10: 1 for layouts 1 - 3, 0 for 4 up
   or a negative one; for layout 0 the two fighters' characters are counted. */
SFA_FUNC(0896134C, count_character_10) {
    int32_t layout = S16(BATTLE + 56);
    RETURN(0);
    if (layout > 0) { c->r[4] = layout < 4; if (layout < 4) RETURN(1); return; }
    if (layout < 0) { c->r[4] = (uint32_t)layout; return; }
    c->r[4] = 0xA;
    if (S8(FIGHTER_1 + 338) == 10) RETURN(1);
    c->r[5] = (uint32_t)(int32_t)S8(FIGHTER_2 + 338);
    if ((int32_t)c->r[5] == 10) RETURN(c->r[2] + 1);
}

/* ---------------------------------------------------------------- the network session */

/* (list): its first item taken off (the second, +0, follows; the last, +4, too if it was the one
   taken), the count +8 one less; answers the item, or 0 when the list is empty. */
SFA_FUNC(088E0698, list_take_first) {
    gaddr l = ARG(0);
    uint32_t n = *W(l + 8);
    if (n == 0) { RETURN(0); return; }
    gaddr item = *W(l);
    uint32_t next = *W(item);
    *W(l) = next;
    if (*W(l + 4) == item) *W(l + 4) = *W(item);
    *W(l + 8) = n - 1;
    RETURN(item);
}

/* (n): the session's place +228 moved by n: 0 past its end (+236), the end when below 0. */
SFA_FUNC(088DD164, session_place_after) {
    int32_t v = (int32_t)(*W(SESSION + 228) + ARG(0)), end = (int32_t)*W(SESSION + 236);
    c->r[4] = (uint32_t)end;
    if (end < v) { RETURN(0); return; }
    RETURN(v < 0 ? end : v);
}

/* (): 1 while this side waits for the host's choice, else the session's end (+236). */
SFA_FUNC(088DD3DC, session_end_unless_waiting) {
    c->r[29] -= 0x10;
    f_088D5E7C(c);
    c->r[4] = 0x09520000u;
    if (c->r[2] != 0) RETURN(1);
    else { c->r[4] = SESSION; RETURN(*W(SESSION + 236)); }
    c->r[29] += 0x10;
}

/* (): 3 when the session is in state 2 or 3 (NET +42) with +44 = 1, else 8. */
SFA_FUNC(088D591C, net_state_3_or_8) {
    uint32_t state = *B(NET + 42);
    c->r[5] = NET; c->r[6] = 2; c->r[4] = state;
    if (state == 3 || state == 2) {
        c->r[4] = *B(NET + 44); c->r[5] = 1;
        if (c->r[4] == 1) { RETURN(3); return; }
    }
    RETURN(8);
}

/* (record): a scan for ad-hoc groups started; the record's +20 = 5 unless it failed. Answers the scan's
   result. */
SFA_FUNC(088DF574, net_scan) {
    gaddr r = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = r;
    int32_t result = os_net_adhocctl_scan(c);
    c->r[4] = 5;
    if (result >= 0) *W(r + 20) = 5;
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (buffer): this PSP's MAC address at +0 and its nickname (system parameter 1, up to 0x80 bytes) at +8. */
SFA_FUNC(088DE8F0, net_my_address_and_name) {
    gaddr buf = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = buf;
    os_net_get_local_ether_addr(c, buf);
    os_utility_get_system_param_string(c, 1, buf + 8, 0x80);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- text */

/* (text, kind, x, y, colour): a text request (game_text) with +3 = 8. */
SFA_FUNC(088E40D8, game_text_8) {
    c->r[29] -= 0x30;
    c->r[5] = ARG(1) & 0xFF;
    c->r[6] = (uint32_t)(int32_t)(int16_t)ARG(2);
    c->r[7] = (uint32_t)(int32_t)(int16_t)ARG(3);
    c->r[8] = c->r[8] & 0xFF;
    f_0880A6CC(c);
    c->r[4] = 8;
    if (c->r[2] != 0) *B(c->r[2] + 3) = 8;
    c->r[29] += 0x30;
}
