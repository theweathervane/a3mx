/* Small steps (0.126.0), from the profile refreshed at 0.125.0: objects moved by their speeds and
   counting down, records cleared, the network session's lists, checksum and flags, a hex digit, a
   fighter's script start and guard flags. Offsets stay numbers until something shows what they are.
   What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "../decomp.h"

#include "../guest_mem.h"

#define SESSION 0x09523820u     /* the network session's records (0x09523920 its state) */

void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_089F3470(Cpu *c);   /* game_memset */
void f_088232B0(Cpu *c);   /* record_reset */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08821368(Cpu *c);   /* game_random */

/* ---------------------------------------------------------------- objects */

/* (object): +16 / +20 moved by +96 / +100. */
SFA_FUNC(0884FEAC, move_by_96_100) {
    gaddr o = ARG(0);
    *W(o + 16) += *W(o + 96);
    *W(o + 20) += *W(o + 100);
}

/* (object): +20 moved by +100, its high half (+22) brought back by 0x1C0 from 353 up. */
SFA_FUNC(08880E74, move_down_wrapping) {
    gaddr o = ARG(0);
    *W(o + 20) += *W(o + 100);
    int16_t y = S16(o + 22);
    if (!(y < 353)) *H(o + 22) = (uint16_t)(y - 0x1C0);
}

/* (object): +16 moved by +96 (back when +11, its facing, is 0), +96 by +104; returns +96. */
SFA_FUNC(08908BA8, move_x_accelerating) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    uint32_t d = *B(o + 11) == 0 ? 0u - v : v;
    *W(o + 16) += d;
    v += *W(o + 104);
    *W(o + 96) = v;
    RETURN(v);
}

/* (object, n): +84 from 0x08A2A098 by n. */
SFA_FUNC(0889858C, set_84_from_table) {
    *H(ARG(0) + 84) = *H(0x08A2A098u + (uint32_t)(int32_t)(int16_t)ARG(1) * 2);
}

/* (n): the half-word the n-th pointer of 0x08A22DD0 points at. */
SFA_FUNC(0887CAD0, halfword_by_pointer) {
    RETURN(*H(*W(0x08A22DD0u + (uint32_t)(int32_t)(int8_t)ARG(0) * 4)));
}

/* (record): +20 counts down; at 0 the next step (+8). */
SFA_FUNC(08826D44, count_20_then_8) {
    gaddr r = ARG(0);
    int32_t v = S16(r + 20);
    if (v != 0) *H(r + 20) = (uint16_t)(v - 1);
    else *H(r + 8) = (uint16_t)(*H(r + 8) + 1);
}

/* (record): +22 counts down; past 0 the next step (+10). */
SFA_FUNC(08831C90, count_22_then_10) {
    gaddr r = ARG(0);
    int16_t v = (int16_t)(S16(r + 22) - 1);
    *H(r + 22) = (uint16_t)v;
    if (v < 0) *H(r + 10) = (uint16_t)(*H(r + 10) + 1);
}

/* (record, object): the record's step 5 for 0xF frames; the object +65 = -1, +82 = 2, +18 = 0, +151 = 1. */
SFA_FUNC(08831FF4, step_5_object_ready) {
    gaddr r = ARG(0), o = ARG(1);
    *H(r + 10) = 5;
    *H(r + 22) = 0xF;
    *B(o + 65) = 0xFF;
    *B(o + 82) = 2;
    *H(o + 18) = 0;
    *B(o + 151) = 1;
}

/* (record): next part (+6), +18 = 0x168, +43 = 1 unless +57 (then +43 and +72 cleared). */
SFA_FUNC(08827ED4, part_with_168) {
    gaddr r = ARG(0);
    *H(r + 6) = (uint16_t)(*H(r + 6) + 1);
    *H(r + 18) = 0x168;
    *B(r + 43) = 1;
    if (S8(r + 57) != 0) { *B(r + 43) = 0; *B(r + 72) = 0; }
}

/* (object): +1 off; +5 counts up, from 3 the next step (+4). */
SFA_FUNC(088D0D34, count_5_then_4) {
    gaddr o = ARG(0);
    *B(o + 1) = 0;
    uint32_t v = *B(o + 5);
    *B(o + 5) = (uint8_t)(v + 1);
    if (!((int32_t)v < 3)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object): +5 from 0 to 1 the first time, then drawn. */
SFA_FUNC(089CE80C, step_once_then_draw) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t v = *B(o + 5);
    c->r[5] = v + 1;
    if (v == 0) *B(o + 5) = (uint8_t)c->r[5];
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +82 a random 0x40 - 0x23F. */
SFA_FUNC(0884E570, random_82) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_08821368(c);
    *H(o + 82) = (uint16_t)((c->r[2] & 0x1FF) + 0x40);
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (object): +102 = 1, +134 from +68, +308 from +70, +103 the top four bits of +66. */
SFA_FUNC(089DFC14, set_from_66_70) {
    gaddr o = ARG(0);
    int16_t a = S16(o + 68), b = S16(o + 66);
    *B(o + 102) = 1;
    *H(o + 134) = (uint16_t)a;
    *W(o + 308) = (uint32_t)(int32_t)S16(o + 70);
    *B(o + 103) = (uint8_t)(((int32_t)b & 0xF000) >> 12);
}

/* (object): returned to its pool with +8 = 7, pool 7's free count (0x095176F4) one more. */
SFA_FUNC(088231F4, object_free_7) {
    c->r[29] -= 0x20;
    c->r[5] = 7;
    f_088232B0(c);
    *B(0x095176F4u) = (uint8_t)(S8(0x095176F4u) + 1);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- tables and records */

/* (): the eleven bytes at 0x0950FF40 cleared. */
SFA_FUNC(08822B88, clear_0950FF40) {
    for (uint32_t i = 0; i < 0xB; i++) *B(0x0950FF40u + i) = 0;
}

/* (): 0x08A2B594 = 0, -1, 1, 2. */
SFA_FUNC(088A063C, set_08A2B594) {
    gaddr t = 0x08A2B594u;
    *B(t) = 0; *B(t + 1) = 0xFF; *B(t + 2) = 1; *B(t + 3) = 2;
}

/* (): the bytes 0x08B03F50 + n = n for n 7 down to 0. */
SFA_FUNC(088E4960, count_08B03F50) {
    for (int32_t n = 7; n >= 0; n--) *B(0x08B03F50u + (uint32_t)n) = (uint8_t)n;
}

/* (fighter, a, b): its +61, a computer fighter (+373), or b's +32 clear turn off b's +55. */
SFA_FUNC(08828890, clear_55_unless) {
    gaddr b = ARG(1), f = ARG(2);
    if ((S8(b + 61) | S8(f + 373) | (S8(b + 32) == 0)) != 0) *B(b + 55) = 0;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, script): the CPU's script count +550 on, then the script read. */
SFA_FUNC(0887A094, fighter_script_550_read) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    c->r[6] = (uint32_t)(S8(f + 550) + 1);
    *B(f + 550) = (uint8_t)c->r[6];
    f_0887A10C(c);
    c->r[29] += 0x20;
}

/* (fighter, script): its script started at the script's word (+506), routine 1 / 0 / 0 / 0; returns the
   script after the word. */
SFA_FUNC(08877610, fighter_script_begin) {
    gaddr f = ARG(0), p = ARG(1);
    *B(f + 483) = 0;
    *B(f + 482) = 0;
    *H(f + 506) = (uint16_t)S16(p);
    *B(f + 4) = 1;
    *B(f + 7) = 0;
    *B(f + 6) = 0;
    *B(f + 5) = 0;
    RETURN(p + 2);
}

/* (fighter): +696 cleared; +117 = 4 with +588, else 5. */
SFA_FUNC(08871904, fighter_117_by_588) {
    gaddr f = ARG(0);
    *B(f + 696) = 0;
    *B(f + 117) = S8(f + 588) != 0 ? 4 : 5;
}

/* (object): whether its owner (+88) has none of +560 - 562 set. */
SFA_FUNC(08880A04, owner_free_560) {
    gaddr f = *W(ARG(0) + 88);
    RETURN((S8(f + 561) | S8(f + 560) | S8(f + 562)) != 0 ? 0 : 1);
}

/* ---------------------------------------------------------------- the network session */

/* (n, out): n as a hexadecimal digit's character. */
SFA_FUNC(088DE3D4, hex_digit) {
    int32_t n = (int32_t)ARG(0);
    *B(ARG(1)) = (uint8_t)(n < 10 ? n + 0x30 : n + 0x37);
}

/* (): the session's 0xF4 bytes cleared. */
SFA_FUNC(088DBF14, session_clear) {
    c->r[29] -= 0x20;
    c->r[4] = SESSION; c->r[5] = 0; c->r[6] = 0xF4;
    f_089F3470(c);
    c->r[29] += 0x20;
}

/* (n): the n-th entry of the session's list (+232, linked by +0), or 0. */
SFA_FUNC(088DD1AC, session_list_nth) {
    int32_t n = (int32_t)ARG(0);
    uint32_t e = *W(SESSION + 232);
    while (n > 0 && e != 0) { n--; e = *W(e); }
    RETURN(e);
}

/* (list, item): the item added at the end of a list (+0 first, +4 last, +8 count); returns the count. */
SFA_FUNC(088E066C, list_append) {
    gaddr l = ARG(0);
    uint32_t item = ARG(1), first = *W(l), last = *W(l + 4);
    if (first == 0) *W(l) = item;
    if (last != 0) *W(last) = item;
    uint32_t n = *W(l + 8) + 1;
    *W(l + 4) = item;
    *W(l + 8) = n;
    RETURN(n);
}

/* (bytes, n): the session's checksum: 0xA0E7949F plus each byte + 1. */
SFA_FUNC(088DC054, session_checksum) {
    gaddr p = ARG(0);
    int32_t n = (int32_t)ARG(1);
    uint32_t sum = 0xA0E7949Fu;
    for (; n > 0; n--, p++) sum += (uint32_t)*B(p) + 1;
    RETURN(sum);
}

/* (flag): with a session record at 0x09523960, its +24 = whether flag (a byte) is set. */
SFA_FUNC(088E3F98, session_flag_24) {
    gaddr s = 0x09523960u;
    if (*W(s) != 0) *B(s + 24) = (int8_t)ARG(0) != 0;
}

/* (): 1 unless the session state's +48 is set and its +46 is under 2. */
SFA_FUNC(088E376C, session_state_ok) {
    gaddr s = 0x09523920u;
    if (S8(s + 48) == 0) { RETURN(1); return; }
    RETURN(!(S8(s + 46) < 2));
}

/* (): 0x08A3EB96 = the state's +265 + 4, 0x08A3EB97 = 2, 0x08A3EBA8 = +266 (0x09523980). */
SFA_FUNC(088E32BC, session_bytes_08A3EB96) {
    gaddr s = 0x09523980u;
    *B(0x08A3EB96u) = (uint8_t)(S8(s + 265) + 4);
    *B(0x08A3EB97u) = 2;
    *B(0x08A3EBA8u) = (uint8_t)S8(s + 266);
}
