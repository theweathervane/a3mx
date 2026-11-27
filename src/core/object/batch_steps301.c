/* More of what the new leaves set free: a second helper fighter's start, a new entry for the link's
   table of peers, the piece that follows characters 17 and 19, and the screen object that carries out
   the frame's requests (fades, scrolls, sound). Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08876998(Cpu *c); void f_0892926C(Cpu *c); void f_0887684C(Cpu *c); void f_089287C8(Cpu *c);
void f_088D595C(Cpu *c); void f_088DC18C(Cpu *c); void f_089F3470(Cpu *c); void f_088DE4A0(Cpu *c);
void f_088DC6A0(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_0894CFD8(Cpu *c);
void f_088233BC(Cpu *c); void f_089DEA64(Cpu *c); void f_089E0254(Cpu *c); void f_089DE9F0(Cpu *c);
void f_08822934(Cpu *c); void f_08823304(Cpu *c); void f_0882222C(Cpu *c); void f_089DFED4(Cpu *c);
void f_089E044C(Cpu *c); void f_08821FEC(Cpu *c); void f_08822444(Cpu *c); void f_08821F9C(Cpu *c);
void f_089DEB18(Cpu *c); void f_089E0324(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (helper): a second helper fighter's start: state 1, its owner's (+88) +24, its own tables (+352,
   +356, +128, +200) and 0x08876998; kinds 6 and up (+346) take their vitality from the table
   0x08A6A594 by the owner's strength, and their script (0x0892926C); at rest on the ground, moving
   by its facing at the speed of its kind (table 0x08A6A598), placed from where it is by the offsets
   0x08A6A5BC picks by +139; then 0x0887684C with the kind and 0x089287C8. A 0x30 frame keeping
   s0..s4. */
SFA_FUNC(08928668, helper_start_2) {
    gaddr x = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = x;
    *W(sp + 20) = sv[1];
    gaddr own = *W(x + 88);
    c->r[17] = own;
    uint32_t kind = (uint32_t)(int32_t)S8(x + 346) & 0xFFFF;
    *B(x + 4) = 1;
    *W(sp + 24) = sv[2];
    *B(x + 9) = 0;
    *H(x + 24) = *H(own + 24);
    *W(x + 352) = 0x08A67694u; *W(x + 356) = 0x08A676BCu; *W(x + 128) = 0x08A67C1Cu; *W(x + 200) = 0x08A67A4Au;
    c->r[18] = kind;
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    call1(c, x, f_08876998, 0x089286E8u);
    gaddr speed_at = 0x08A6A598u + (kind << 2);
    c->r[19] = speed_at; c->r[20] = 0x08A6A5BCu;
    if (!((int32_t)kind < 6)) {
        *H(x + 112) = (uint16_t)(int16_t)S8(0x08A6A594u + (uint32_t)(int32_t)S8(own + 194));
        call1(c, x, f_0892926C, 0x08928730u);
    }
    *W(x + 100) = 0; *B(x + 73) = 0;
    uint8_t facing = *B(x + 11);
    int32_t k = S8(x + 139);
    uint32_t speed = *W(speed_at);
    int32_t px = S16(x + 18), py = S16(x + 22);
    gaddr e = 0x08A6A5BCu + (uint32_t)(k + k);
    if (facing != 0) { *W(x + 96) = speed; *H(x + 18) = (uint16_t)(px + S16(e)); }
    else { *W(x + 96) = 0 - speed; *H(x + 18) = (uint16_t)(px - S16(e)); }
    c->r[19] = (uint32_t)(py + S16(e + 2));
    *H(x + 22) = (uint16_t)c->r[19];
    call2(c, x, kind, f_0887684C, 0x089287A0u);
    call1(c, x, f_089287C8, 0x089287A8u);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (address, value): a new entry in the link's table of peers (0x09523720: first, last, a lock, the
   count at +14), under the lock: while the count is below 0x088D595C's limit less one, 0x9C bytes
   (0x088DC18C), cleared, +152 = 0x2D0, the six address bytes at +4, the value at +10, the address's
   name at +12 (0x088DE4A0); when 0x088DC6A0 already knows the address its entry is returned instead;
   else the new one is hung at the end and counted. The entry, or 0. A 0x30 frame keeping s0..s4. */
SFA_FUNC(088DC53C, net_peer_add) {
    gaddr addr = ARG(0); uint32_t v = ARG(1) & 0xFFFF;
    const gaddr tab = 0x09523720u;
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[17] = tab; c->r[18] = addr;
    uint32_t lock = *W(tab + 8);
    *W(sp + 28) = sv[3]; c->r[19] = v;
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    c->r[16] = 0;
    if (lock != 0) { c->r[6] = 0; call2(c, lock, 1, hle_sceKernelWaitSema, 0x088DC580u); }
    c->r[20] = (uint32_t)(int32_t)S16(tab + 14);
    c->r[31] = 0x088DC588u; f_088D595C(c);
    if ((int32_t)c->r[20] < (int32_t)(c->r[2] - 1)) {
        call1(c, 0x9C, f_088DC18C, 0x088DC5A0u);
        c->r[16] = c->r[2];
    }
    lock = *W(tab + 8);
    if (lock != 0) call2(c, lock, 1, hle_sceKernelSignalSema, 0x088DC5B8u);
    gaddr n = c->r[16];
    if (n != 0) {
        c->r[6] = 0x9C;
        call2(c, n, 0, f_089F3470, 0x088DC5CCu);
        *H(n + 152) = 0x2D0;
        int8_t a0 = S8(addr), a1 = S8(addr + 1), a2 = S8(addr + 2);
        *B(n + 4) = (uint8_t)a0;
        int8_t a3 = S8(addr + 3);
        *B(n + 5) = (uint8_t)a1;
        int8_t a4 = S8(addr + 4);
        *B(n + 6) = (uint8_t)a2;
        int8_t a5 = S8(addr + 5);
        *B(n + 7) = (uint8_t)a3; *B(n + 8) = (uint8_t)a4; *B(n + 9) = (uint8_t)a5;
        *H(n + 10) = (uint16_t)v;
        call2(c, addr, n + 0xC, f_088DE4A0, 0x088DC618u);
        call1(c, addr, f_088DC6A0, 0x088DC620u);
        if (c->r[2] != 0) c->r[16] = c->r[2];
        else {
            lock = *W(tab + 8);
            if (lock != 0) { c->r[6] = 0; call2(c, lock, 1, hle_sceKernelWaitSema, 0x088DC644u); }
            uint32_t first = *W(tab), last = *W(tab + 4);
            if (first == 0) *W(tab) = n;
            if (last != 0) *W(last) = n;
            int32_t count = S16(tab + 14);
            *W(tab + 4) = n;
            lock = *W(tab + 8);
            *H(tab + 14) = (uint16_t)(count + 1);
            if (lock != 0) call2(c, lock, 1, hle_sceKernelSignalSema, 0x088DC67Cu);
        }
    }
    c->r[2] = c->r[16];
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (piece): the piece that follows characters 17 and 19, by its state (+4): 0 takes its owner's (+88)
   looks (+12..+14, +24, +164, its side into +86) and goes to 1; 1 follows as the pieces in
   batch_steps298 do (by pixel, +78 keeps the number, the table 0x08A768A8), or goes to 2 when the owner
   is neither character; 2 goes to 3; 3 frees itself from the owner (0x0894CFD8). */
SFA_FUNC(0894CD38, piece_follow_768a8) {
    gaddr x = ARG(0);
    uint32_t st = *B(x + 4);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr own = *W(x + 88);
    c->r[5] = own;
    if (st == 0) {
        *B(x + 4) = 1;
        *H(x + 24) = *H(own + 24);
        *B(x + 12) = (uint8_t)S8(own + 12);
        *B(x + 13) = *B(own + 13);
        uint8_t b14 = *B(own + 14);
        *B(x + 78) = 0;
        *B(x + 14) = b14;
        *H(x + 86) = (uint16_t)(int16_t)S8(own + 337);
        uint32_t w164 = *W(own + 164);
        st = *B(x + 4);
        *W(x + 164) = w164;
    }
    if (st == 1) {
        *B(x + 1) = 0;
        int32_t who = S8(own + 338);
        if (who != 0x11 && who != 0x13) *B(x + 4) = (uint8_t)(st + 1);
        else if (*B(*W(own + 296) + 10) == 0) *B(x + 78) = 0;
        else {
            *H(x + 18) = (uint16_t)S16(own + 18);
            *H(x + 22) = (uint16_t)S16(own + 22);
            uint8_t facing = *B(own + 11);
            *B(x + 1) = 1; *B(x + 11) = facing;
            uint32_t k = *B(*W(own + 296) + 10);
            int32_t had = S8(x + 78);
            if ((uint32_t)had == k) { c->r[31] = 0x0894CE58u; f_088219B0(c); }
            else {
                *B(x + 78) = (uint8_t)k;
                uint32_t now = *B(*W(own + 296) + 10);
                call2(c, x, *W(0x08A768A8u + (now << 2)), f_0882196C, 0x0894CE44u);
            }
        }
    } else if (st == 2) *B(x + 4) = (uint8_t)(st + 1);
    else if (st == 3) { c->r[31] = 0x0894CE90u; f_0894CFD8(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (screen): carries out the requests other code left on the screen object this frame, each flag
   cleared as it is served: +99 (GAME + 93 / 94 cleared, a pending 0x09525E04 handed to 0x088233BC,
   then 0x089DEA64); +101 (GAME + 2 on); the four values +126..+132 taken into +24..+30 when they
   differ (0x089E0254 with the new and the old); +102 (0x089DE9F0); +104 (0x08822934, 0x08823304,
   0x0882222C, 0x089DFED4, 0x089E044C); +105; +106 by its number (1: 0x08821FEC and 0x089E044C, 2:
   0x08822444, 4: 0x08821F9C and 0x089E044C); +107 (0x089DEB18); +100 (0x089E0254 with +150); +98
   (0x089E0324 with +110); +109. */
SFA_FUNC(089DF624, screen_requests_serve) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    int8_t first = S8(o + 99);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (first != 0) {
        *B(GAME + 93) = 0; *B(GAME + 94) = 0;
        if (*B(0x09525E00u) != 0) {
            uint32_t what = *W(0x09525E04u);
            *B(0x09525E00u) = 0;
            call1(c, what, f_088233BC, 0x089DF66Cu);
        }
        *B(o + 99) = 0;
        call1(c, o, f_089DEA64, 0x089DF678u);
    }
    if (S8(o + 101) != 0) {
        *B(o + 101) = 0;
        *H(GAME + 2) = (uint16_t)(*H(GAME + 2) + 1);
    }
    static const uint32_t backs[4] = { 0x089DF6B0u, 0x089DF6C8u, 0x089DF6E0u, 0x089DF6F8u };
    for (uint32_t i = 0; i < 4; i++) {
        int16_t have = S16(o + 24 + 2 * i), want = S16(o + 126 + 2 * i);
        if (have != want) {
            *H(o + 24 + 2 * i) = (uint16_t)want;
            call2(c, (uint32_t)(int32_t)want, (uint32_t)(int32_t)have, f_089E0254, backs[i]);
        }
    }
    if (S8(o + 102) != 0) {
        *B(o + 102) = 0;
        call1(c, o, f_089DE9F0, 0x089DF710u);
    }
    if (S8(o + 104) != 0) {
        c->r[31] = 0x089DF724u; f_08822934(c);
        c->r[31] = 0x089DF72Cu; f_08823304(c);
        c->r[31] = 0x089DF734u; f_0882222C(c);
        call1(c, o, f_089DFED4, 0x089DF73Cu);
        *B(o + 104) = 0;
        call1(c, o, f_089E044C, 0x089DF748u);
    }
    int8_t f105 = S8(o + 105);
    int32_t k = S8(o + 106);
    if (f105 != 0) *B(o + 105) = 0;
    if (k != 0) {
        if (k == 1) {
            c->r[31] = 0x089DF780u; f_08821FEC(c);
            call1(c, o, f_089E044C, 0x089DF788u);
        } else if (k == 2) { c->r[31] = 0x089DF7ACu; f_08822444(c); }
        else if (k == 4) {
            c->r[31] = 0x089DF7BCu; f_08821F9C(c);
            call1(c, o, f_089E044C, 0x089DF7C4u);
        }
        *B(o + 106) = 0;
    }
    if (S8(o + 107) != 0) {
        *B(o + 107) = 0;
        call1(c, o, f_089DEB18, 0x089DF7E0u);
    }
    if (S8(o + 100) != 0) {
        int16_t v = S16(o + 150);
        *B(o + 100) = 0;
        call1(c, (uint32_t)(int32_t)v, f_089E0254, 0x089DF7F8u);
    }
    if (S8(o + 98) != 0) {
        call1(c, (uint32_t)(int32_t)S16(o + 110), f_089E0324, 0x089DF80Cu);
        *B(o + 98) = 0;
    }
    if (S8(o + 109) != 0) *B(o + 109) = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
