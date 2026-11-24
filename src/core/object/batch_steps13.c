/* Small steps (0.142.0): the 3 functions of 9 - 60 instructions made ready by the batch before and the
   next 8 of 61 - 100. The gauge's digits object, followers stepping their owner's records, objects
   moving to a target and falling away, round markers, a row object's shades, and the network
   thread's starts and entry, and a name from the MAC address. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 - t1 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME       0x08E75910u
#define NET_THREAD 0x09523740u

void f_088DE924(Cpu *c);   /* net_thread_loop */
void f_088812AC(Cpu *c);   /* gauge_digits */
void f_08881D9C(Cpu *c);   /* setup_kind_at */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088DE3D4(Cpu *c);   /* hex_digit */
void f_089F3470(Cpu *c);   /* game_memset */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_0887CA88(Cpu *c);   /* fighter_layout_record */
void f_0887CAF4(Cpu *c);   /* next_of_128 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_08822F1C(Cpu *c);   /* object_make_16 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_088DF288(Cpu *c);   /* record_set_name */
void f_08821044(Cpu *c);   /* object_move */
void f_088D5E44(Cpu *c);   /* game_network_battle */

/* ---------------------------------------------------------------- the gauge's digits */

/* (object): the digits object's start: +112 cleared, set up (kind 0 at 0, 0x8C), its rows 0x08A29888
   kept at 0x092E3E98, +148 = 1, the digits drawn and animation 0x08A29A24. */
SFA_FUNC(088811C8, gauge_digits_start) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 112) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; c->r[5] = 0; c->r[6] = 0; c->r[7] = 0x8C; f_08881D9C(c);
    *W(0x092E3E98u) = 0x08A29888u;
    *W(o + 148) = 1;
    c->r[4] = o; f_088812AC(c);
    c->r[4] = o; c->r[5] = *W(0x08A29A24u); f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): the digits object's step: ended (+4 on) when GAME +69 is below 0 or +42 clear; else 0xA000
   further down (+16), the digits and drawn. */
SFA_FUNC(08881240, gauge_digits_step) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(GAME + 69) >= 0 && S8(GAME + 42) != 0) {
        *W(o + 16) += 0xA000;
        c->r[4] = o; f_088812AC(c);
        c->r[4] = o; f_088215C4(c);
    } else {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- objects */

/* (object): a follower: first its owner's (+88) +24 and +171 taken (layer 2) and a layout record
   stepped; then each time +82 runs out, the next one, ending (+4 = 2) once the owner's routine
   (+4 / +5 / +6 as the top three bytes) is no longer the one kept at +100; drawn while the owner's +1
   is set. Freed from step 2. */
SFA_FUNC(0887C978, follower_by_owner_routine) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    if (st == 0) {
        *B(o + 4) = 1;
        gaddr own = *W(o + 88);
        *B(o + 9) = 2;
        *H(o + 24) = *H(own + 24);
        *B(o + 171) = (uint8_t)S8(own + 171);
        c->r[4] = o; f_0887CA88(c);
        c->r[4] = o; c->r[5] = c->r[2]; f_0887CAF4(c);
    } else if (st == 1) {
        int16_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n < 0) {
            gaddr own = *W(o + 88);
            c->r[17] = own;
            *H(o + 82) = 0;
            uint32_t key = (uint32_t)*B(own + 4) << 24 | (uint32_t)*B(own + 5) << 16 | (uint32_t)*B(own + 6) << 8;
            if (key != *W(o + 100)) *B(o + 4) = 2;
            c->r[4] = o; f_0887CA88(c);
            c->r[4] = o; c->r[5] = c->r[2]; f_0887CAF4(c);
            if (*B(own + 1) != 0) { c->r[4] = o; f_088215C4(c); }
        }
    } else if (st == 2) {
        c->r[4] = o; f_088231C8(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): lit (+15), animation by +3 from 0x08A165A8, drawn; thrown (speeds -0x15000 / 0x7000, pulls
   0 / -0x800), +112 -1; an object of kind 0x4F / 7 made below it at 0x30 (layer 1, +14 2, +24 7, +38 6,
   +86 4, +148 7, +164 1, +160 0xFF, following it). Drawn again. */
SFA_FUNC(08853CC0, thrown_with_shadow) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[18] = 1; c->r[17] = 4;
    *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0;
    *H(o + 86) = 4;
    c->r[4] = o; c->r[5] = *W(0x08A165A8u + *B(o + 3) * 4); f_0882196C(c);
    c->r[4] = o; f_08821424(c);
    *W(o + 96) = 0xFFFEB000u;
    *W(o + 104) = 0;
    *W(o + 100) = 0x7000;
    *W(o + 108) = 0xFFFFF800u;
    *B(o + 116) = 0;
    *H(o + 112) = 0xFFFF;
    c->r[4] = 0x4F; c->r[5] = 7; f_08822F1C(c);
    gaddr e = c->r[2];
    if (e != 0) {
        *H(e + 18) = (uint16_t)S16(o + 18);
        *H(e + 22) = 0x30;
        *B(e + 78) = 1; *B(e + 11) = 0; *B(e + 14) = 2;
        *H(e + 26) = 0; *H(e + 24) = 7;
        *B(e + 9) = 1;
        *H(e + 38) = 6; *H(e + 86) = 4;
        *W(e + 148) = 7; *W(e + 164) = 1;
        *W(e + 88) = o;
        *B(e + 160) = 0xFF;
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (object, timer): first waits until timer +2 reaches +120; then moves (speeds +96 / +100 with pulls
   +104 / +108) and stops each axis at its target (+112 / +114); once neither moves, +5 on. Drawn. */
SFA_FUNC(089CD800, move_to_target_after) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t s = *B(o + 5);
    if (s == 0) {
        if (!(S16(ARG(1) + 2) < (int16_t)*B(o + 120))) *B(o + 5) = 1;
    } else if (s == 1) {
        c->r[4] = o; f_08821044(c);
        int moving = 0;
        int32_t sx = (int32_t)*W(o + 96);
        if (*W(o + 96) + *W(o + 104) != 0) {
            int16_t x = S16(o + 18), tx = S16(o + 112);
            moving = 1;
            if (!(sx >= 0 ? x < tx : tx < x)) { *H(o + 18) = (uint16_t)tx; *W(o + 104) = 0; *W(o + 96) = 0; }
        }
        int32_t sy = (int32_t)*W(o + 100);
        if (*W(o + 100) + *W(o + 108) != 0) {
            int16_t y = S16(o + 22), ty = S16(o + 114);
            moving = 1;
            if (!(sy >= 0 ? y < ty : ty < y)) { *H(o + 22) = (uint16_t)ty; *W(o + 108) = 0; *W(o + 100) = 0; }
        }
        if (!moving) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): +10 on, +22 = 0x17, the record's +65 cleared and +18 = -1; two markers (list 3,
   kind 1 / 4, +124) and two objects (kind 0xE / 6, +92) for the record's fighters +136 and +140; in a
   network battle one of each, for the human of the two (+136 unless it is the CPU, +373). */
SFA_FUNC(08831B68, round_markers) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = r;
    *H(o + 10) = (uint16_t)(*H(o + 10) + 1);
    *H(o + 22) = 0x17;
    *B(r + 65) = 0;
    *H(r + 18) = 0xFFFF;
    c->r[4] = 0xFFFFFFFFu;
    f_088D5E44(c);
    if (c->r[2] == 0) {
        gaddr m;
        c->r[4] = 1; c->r[5] = 4; f_08823338(c); m = c->r[2];
        if (m == 0) goto done;
        *W(m + 124) = *W(r + 136);
        c->r[4] = 1; c->r[5] = 4; f_08823338(c); m = c->r[2];
        if (m == 0) goto done;
        *W(m + 124) = *W(r + 140);
        c->r[4] = 0xE; c->r[5] = 6; f_08822EAC(c); m = c->r[2];
        if (m == 0) goto done;
        *W(m + 92) = *W(r + 136);
        c->r[4] = 0xE; c->r[5] = 6; f_08822EAC(c); m = c->r[2];
        if (m == 0) goto done;
        *W(m + 92) = *W(r + 140);
    } else {
        gaddr f = *W(r + 136);
        if (S8(f + 373) == 0) f = *W(r + 140);
        c->r[17] = f;
        c->r[4] = 1; c->r[5] = 4; f_08823338(c);
        if (c->r[2] == 0) goto done;
        *W(c->r[2] + 124) = f;
        c->r[4] = 0xE; c->r[5] = 6; f_08822EAC(c);
        if (c->r[2] == 0) goto done;
        *W(c->r[2] + 92) = f;
    }
done:
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): a row object's look by +102: colour 7 / 0xA, +1 = 8, size 0xB0 x 0x10; its three shade sets
   (+36 - +39, +42 - +45, +48 - +51) 0 / FF / 0 for 0, 0 / FF / FF for 1, FF / FF / 0 for 2. */
SFA_FUNC(08888F00, shade_by_102) {
    gaddr o = ARG(0);
    *B(o + 55) = 7; *B(o + 1) = 8; *B(o + 56) = 0xA; *B(o + 3) = 0; *B(o + 136) = 0xFF;
    *H(o + 12) = 0; *H(o + 24) = 0; *H(o + 98) = 0xFF;
    int16_t k = S16(o + 102);
    *H(o + 94) = 0;
    static const uint8_t sets[3][3] = { { 0, 0xFF, 0 }, { 0, 0xFF, 0xFF }, { 0xFF, 0xFF, 0 } };
    if (k >= 0 && k <= 2) {
        for (int s = 0; s < 3; s++)
            for (int i = 0; i < 4; i++) *B(o + 36 + s * 6 + i) = sets[k][s];
        *H(o + 14) = 0xB0; *H(o + 26) = 0x10;
    }
}

/* ---------------------------------------------------------------- the network */

/* (out, mac): an 8-character name from a MAC address: the sum of its first three bytes in two hex
   digits, then bytes 3 - 5 in two each. */
SFA_FUNC(088DF2F4, name_from_mac) {
    gaddr out = ARG(0), mac = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21], s22 = c->r[22];
    c->r[29] -= 0x40;
    gaddr d0 = c->r[29] + 0x10, d1 = c->r[29] + 0x11;
    c->r[20] = mac; c->r[16] = out; c->r[19] = d0; c->r[18] = d1;
    c->r[4] = out; c->r[5] = 0; c->r[6] = 8; f_089F3470(c);
    uint32_t sum = (*B(mac) + *B(mac + 1) + *B(mac + 2)) & 0xFF;
    gaddr p = out;
    for (int i = 2; i < 6; i++) {
        uint32_t v = i == 2 ? sum : *B(mac + i);
        if (i > 2) c->r[22] = mac + i;
        c->r[4] = (v >> 4) & 0xFF; c->r[5] = d0; f_088DE3D4(c);
        c->r[4] = v & 0xF; c->r[5] = d1; f_088DE3D4(c);
        *B(p) = (uint8_t)S8(d0);
        *B(p + 1) = (uint8_t)S8(d1);
        p += 2;
    }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21; c->r[22] = s22;
}

/* A network thread made (`name`, `entry`, priority 0x40, 0x2000 stack) and started: +24 = value, its
   record named when `rec_name` is given, +188 set; 0 then. If either fails: the result into +4 when it
   is still 0, the thread's info freed and the thread deleted, +0 = -1, +24 cleared, the result. */
static void thread_start_with(Cpu *c, uint32_t name, uint32_t entry) {
    gaddr t = NET_THREAD, rec_name = ARG(0), value = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x40;
    c->r[19] = rec_name; c->r[18] = value; c->r[17] = t;
    int32_t r = os_create_thread(c, name, entry, 0x40, 0x2000, 0, 0);
    c->r[16] = (uint32_t)r;
    int ok = 0;
    if (r >= 0) {
        *W(t) = (uint32_t)r;
        *W(t + 24) = value;
        if (rec_name != 0) { c->r[4] = t; c->r[5] = rec_name; f_088DF288(c); }
        *B(t + 188) = 1;
        r = os_start_thread(c, (int32_t)*W(t), 0, 0);
        ok = r >= 0;
    }
    if (ok) RETURN(0);
    else {
        if (*W(t + 4) == 0) *W(t + 4) = (uint32_t)r;
        if ((int32_t)*W(t) >= 0) {
            c->r[4] = *W(t); hle_sceNetFreeThreadinfo(c);
            os_delete_thread(c, (int32_t)*W(t));
        }
        *W(t) = 0xFFFFFFFFu;
        *W(t + 24) = 0;
        RETURN((uint32_t)r);
    }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (name, value): thread_start_with the thread 0x08A012F0, entry 0x088DFC34. */
SFA_FUNC(088DEB60, net_thread_start_dfc34) { thread_start_with(c, 0x08A012F0u, 0x088DFC34u); }
/* (name, value): thread_start_with the thread 0x08A0131C, entry 0x088DEAC4. */
SFA_FUNC(088DEEC4, net_thread_start_deac4) { thread_start_with(c, 0x08A0131Cu, 0x088DEAC4u); }

/* (): the network thread's entry: its loop; then its ad hoc control handler (+12) removed, and ad hoc
   control, ad hoc and the network ended (+193 / +192 / +191 cleared). */
SFA_FUNC(088DEA18, net_thread_entry) {
    gaddr t = NET_THREAD, s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = t;
    f_088DE924(c);
    if ((int32_t)*W(t + 12) >= 0) {
        os_net_adhocctl_del_handler(c, (int32_t)*W(t + 12));
        *W(t + 12) = 0xFFFFFFFFu;
    }
    os_net_adhocctl_term(c);
    *B(t + 193) = 0;
    os_net_adhoc_term(c);
    *B(t + 192) = 0;
    os_net_term(c);
    *B(t + 191) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16;
}
