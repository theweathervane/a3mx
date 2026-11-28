/* The link play's upper layers: the check that every peer answered with the same word, a message's
   send by its kind, the thread that brings the wireless link up, and the exchange of both sides'
   settings before a match. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define LINK 0x09523740u /* the link's state */
#define SWAP 0x09523AA0u /* the settings packet being exchanged: a sum, a mark, 0xE0 bytes */
#define NETP 0x09523980u /* the link play's own state */
#define SETS 0x09523438u /* each side's settings, 208 bytes each */
#define OPTS 0x09523C00u /* the nine option bytes the first side decides */

void f_088DD3DC(Cpu *c); void f_088DD418(Cpu *c); void f_088E0C6C(Cpu *c); void f_088E1094(Cpu *c);
void f_088E233C(Cpu *c); void f_088DC53C(Cpu *c); void f_088DC6A0(Cpu *c); void f_088DCA5C(Cpu *c);
void f_088DC2C4(Cpu *c); void f_088DD22C(Cpu *c); void f_088DE388(Cpu *c); void f_088DE8F0(Cpu *c);
void f_088DF10C(Cpu *c); void f_088DF2F4(Cpu *c); void f_088DF4BC(Cpu *c); void f_088DF500(Cpu *c);
void f_088DF574(Cpu *c); void f_088DF5B4(Cpu *c); void f_0880A6CC(Cpu *c); void f_088D5680(Cpu *c);
void f_088D67C4(Cpu *c); void f_088DC054(Cpu *c); void f_088DD114(Cpu *c); void f_088DD124(Cpu *c);
void f_088DF778(Cpu *c); void f_088DF910(Cpu *c); void f_088DFA48(Cpu *c); void f_088DFC10(Cpu *c);
void f_088E3F44(Cpu *c); void f_089F3430(Cpu *c); void f_089F3470(Cpu *c);
void hle_sceNetInit(Cpu *c); void hle_sceNetAdhocInit(Cpu *c); void hle_sceNetAdhocctlInit(Cpu *c);
void hle_sceNetAdhocctlAddHandler(Cpu *c); void hle_sceKernelDelayThread(Cpu *c);
void hle_sceKernelGetThreadId(Cpu *c); void hle_sceNetFreeThreadinfo(Cpu *c); void hle_sceKernelExitThread(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (x): have all the peers answered alike? Each peer in turn (the walk 0x088DD3DC / 0x088DD418) has
   its message read (0x088E1094 from its +228): with none left to ask, 0x088E233C decides; one that
   has nothing yet reports through 0x088E0C6C (0) and counts a wait at +28. The first whose word is
   0x1CE14355 ends it: every peer's +212 goes up with +214..+218 cleared, and +43 is set with +30,
   +33, +42 and +46 cleared. A 0x40 frame keeping s0..s4; the word last seen lives at +24 of it. */
SFA_FUNC(088E2694, link_all_agree) {
    gaddr x = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 5; i++) *W(sp + 28 + 4 * i) = sv[i];
    *W(sp + 48) = sv[5];
    c->r[16] = x;
    call0(c, f_088DD3DC, 0x088E26B8u);
    *W(sp + 20) = c->r[2];
    const uint32_t word = 0x1CE14355u;
    c->r[18] = word; c->r[20] = sp + 0x14; c->r[19] = sp + 0x10;
    c->r[17] = *W(sp + 24);
    for (;;) {
        call1(c, sp + 0x14, f_088DD418, 0x088E26D8u);
        gaddr peer = c->r[2];
        if (peer == 0) {
            *W(sp + 24) = c->r[17];
            call1(c, x, f_088E233C, 0x088E27BCu);
            break;
        }
        call2(c, *W(peer + 228), sp + 0x10, f_088E1094, 0x088E26F0u);
        if ((int32_t)c->r[2] <= 0) {
            *W(sp + 24) = c->r[17];
            call1(c, 0, f_088E0C6C, 0x088E2784u);
            *H(x + 28) = (uint16_t)(*H(x + 28) + 1);
            c->r[2] = 0;
            break;
        }
        uint32_t got = *W(*W(sp + 16));
        c->r[17] = word;
        if (got != word) continue;
        *W(sp + 24) = word;
        ARG(0) = got;
        call0(c, f_088DD3DC, 0x088E2710u);
        *W(sp + 20) = c->r[2];
        for (;;) {
            call1(c, sp + 0x14, f_088DD418, 0x088E271Cu);
            peer = c->r[2];
            if (peer == 0) break;
            uint16_t n = *H(peer + 212);
            *H(peer + 214) = 0;
            *H(peer + 212) = (uint16_t)(n + 1);
            *H(peer + 218) = 0; *H(peer + 216) = 0;
            *H(x + 30) = 0; *B(x + 33) = 0; *B(x + 42) = 0;
        }
        *B(x + 43) = 1; *B(x + 46) = 0;
        c->r[2] = 0;
        break;
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x40;
}

/* (-, kind, peer, a, b): a message for a peer by its kind, 1 to 10. The peer's link is the one it
   has (0x088DC6A0) or a new one (0x088DC53C, 0x401); with neither, or another kind, nothing. Then
   0x088DCA5C sends on it: the channel by kind (1, 3, 4, 4, 4, 5, 5, 4, 4, 4), the timeout 0x2D0 for
   the first seven kinds and none for the last three, then b and a. A 0x40 frame keeping s0..s3. */
SFA_FUNC(088DE4BC, link_send_kind) {
    static const uint8_t channel[10] = { 1, 3, 4, 4, 4, 5, 5, 4, 4, 4 };
    uint32_t kind = ARG(1), peer = ARG(2), a = ARG(3), b = c->r[8];
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 4; i++) *W(sp + 32 + 4 * i) = sv[i];
    c->r[19] = peer; c->r[16] = b; c->r[17] = a; c->r[18] = kind;
    *W(sp + 48) = sv[4];
    call1(c, peer, f_088DC6A0, 0x088DE4ECu);
    gaddr link = c->r[2];
    if (link == 0) {
        call2(c, peer, 0x401, f_088DC53C, 0x088DE504u);
        link = c->r[2];
    }
    if (link != 0) {
        uint32_t k = kind - 1;
        c->r[18] = k;
        if (k < 10) {
            c->r[18] = k << 2;
            ARG(0) = link; ARG(1) = channel[k]; ARG(2) = k < 7 ? 0x2D0 : 0; ARG(3) = b; c->r[8] = a;
            c->r[31] = 0x088DE54Cu + k * 0x1C;
            f_088DCA5C(c);
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* The thread that brings the wireless link up. LINK + 188 is set while it runs. Whatever is not up
   yet is started and marked: the network (+191), the ad hoc library (+192) and its control (+193;
   "already started", 0x80410B07, counts as up for these two) and the event handler 0x088DE6B8
   (+12). Unless +24 is 2 the old group is left first (0x088DF574, state +20 = 5, polled by
   0x088DF5B4 every second); then the new one is made or joined (0x088DF4BC, state 1, polled by
   0x088DF500), 0x088DE8F0 runs, and for +24 other than 0 or 2 every peer on the list +184 must pass
   0x088DE388 against what 0x088DF2F4 builds, or the result is 0x81001006. The list is freed, the
   state is 2 and with +24 set 0x088DF10C runs. The first result is kept at +4, and the thread frees
   its network data and exits with it. A 0x50 frame keeping s0..s5. */
SFA_FUNC(088DFC34, link_up_thread) {
    const gaddr n = LINK;
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 6; i++) *W(sp + 40 + 4 * i) = sv[i];
    *W(sp + 64) = sv[6];
    c->r[16] = n;
    call0(c, f_088DD22C, 0x088DFC60u);
    c->r[18] = 1;
    uint32_t own = c->r[2];
    c->r[17] = own;
    int32_t res;
    int net_up = S8(n + 191) != 0;
    *B(n + 188) = 1;
    if (!net_up) {
        ARG(1) = 0x20; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 0;
        call1(c, 0x20000, hle_sceNetInit, 0x088DFC8Cu);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res < 0) goto done;
        *B(n + 191) = 1;
    }
    if (S8(n + 192) == 0) {
        ARG(0) = 0;
        call0(c, hle_sceNetAdhocInit, 0x088DFCB0u);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res < 0 && (uint32_t)res != 0x80410B07u) goto done;
        *B(n + 192) = 1;
    }
    if (S8(n + 193) == 0) {
        call3(c, 0x2000, 0x30, 0x08A3EB68u, hle_sceNetAdhocctlInit, 0x088DFCF0u);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res < 0 && (uint32_t)res != 0x80410B07u) goto done;
        *B(n + 193) = 1;
    }
    if ((int32_t)*W(n + 12) < 0) {
        call2(c, 0x088DE6B8u, 0, hle_sceNetAdhocctlAddHandler, 0x088DFD2Cu);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res < 0) goto done;
        *W(n + 12) = (uint32_t)res;
    }
    c->r[20] = 2;
    if (*W(n + 24) == 2) *W(n + 20) = 1;
    else {
        call1(c, n, f_088DF574, 0x088DFD54u);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res < 0) goto done;
        *W(n + 20) = 5;
        c->r[21] = 1000000;
        for (;;) {
            call1(c, n, f_088DF5B4, 0x088DFD74u);
            res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
            if (res != 0) break;
            call1(c, 1000000, hle_sceKernelDelayThread, 0x088DFD88u);
        }
        if (res < 0) goto done;
        *W(n + 20) = 1;
    }
    call1(c, n, f_088DF4BC, 0x088DFDA4u);
    res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
    if (res < 0) goto done;
    *W(n + 20) = 1;
    c->r[18] = 1000000;
    for (;;) {
        call1(c, n, f_088DF500, 0x088DFDC4u);
        res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
        if (res != 0) break;
        call1(c, 1000000, hle_sceKernelDelayThread, 0x088DFDD8u);
    }
    if (res < 0) goto done;
    call1(c, own, f_088DE8F0, 0x088DFDF0u);
    {
        int32_t mode = (int32_t)*W(n + 24);
        gaddr list = *W(n + 184);
        if (mode > 0 && mode != 2 && list != 0) {
            c->r[18] = sp + 0x20;
            call2(c, sp + 0x20, own, f_088DF2F4, 0x088DFE1Cu);
            gaddr at = *W(n + 184);
            c->r[19] = at;
            while (at != 0) {
                call2(c, at + 8, sp + 0x20, f_088DE388, 0x088DFE34u);
                if (c->r[2] == 0) { res = (int32_t)0x81001006u; c->r[19] = (uint32_t)res; goto done; }
                at = *W(at);
                c->r[19] = at;
            }
            list = *W(n + 184);
        }
        if (list != 0) {
            call1(c, list, f_088DC2C4, 0x088DFE64u);
            *W(n + 184) = 0;
        }
    }
    {
        uint32_t mode = *W(n + 24);
        *W(n + 20) = 2;
        if (mode != 0) {
            call2(c, n, own, f_088DF10C, 0x088DFE84u);
            res = (int32_t)c->r[2]; c->r[19] = (uint32_t)res;
            if (res < 0) goto done;
        }
    }
    res = 0; c->r[19] = 0;
done:
    {
        uint32_t first = *W(n + 4);
        gaddr list = *W(n + 184);
        if (first == 0) *W(n + 4) = (uint32_t)res;
        if (list != 0) {
            call1(c, list, f_088DC2C4, 0x088DFEB4u);
            *W(n + 184) = 0;
        }
    }
    *B(n + 188) = 0;
    call0(c, hle_sceKernelGetThreadId, 0x088DFEC0u);
    call1(c, c->r[2], hle_sceNetFreeThreadinfo, 0x088DFEC8u);
    call1(c, (uint32_t)res, hle_sceKernelExitThread, 0x088DFED0u);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x50;
}

/* One side's 208 bytes of settings out of the packet, as words. */
static void keep_side(uint32_t side) {
    gaddr to = SETS + side * 208;
    for (uint32_t i = 0; i < 208; i += 8) {
        uint32_t lo = *W(SWAP + 8 + i), hi = *W(SWAP + 12 + i);
        *W(to + i) = lo; *W(to + i + 4) = hi;
    }
}

/* The first side's nine option bytes go where the game reads them, and the link play's +265 / +266
   say whether more than one peer is there (1 and 0x18) for kinds 2 and 3. `r20` is what s4 holds
   by the count's call. */
static void take_options(Cpu *c, uint32_t r20, int r20_early, uint32_t back_copy, uint32_t back_count) {
    call3(c, OPTS, SWAP + 0xD8, 9, f_089F3430, back_copy);
    if (r20_early) c->r[20] = r20;
    *B(NETP + 45) = *B(OPTS);
    *B(GAME + 73) = *B(OPTS + 1); *B(GAME + 45) = *B(OPTS + 1);
    *B(GAME + 47) = *B(OPTS + 2);
    *B(GAME + 77) = *B(OPTS + 3); *B(GAME + 76) = *B(OPTS + 3);
    *B(GAME + 46) = *B(OPTS + 4); *B(GAME + 116) = *B(OPTS + 4);
    *B(GAME + 48) = *B(OPTS + 5);
    *B(GAME + 118) = *B(OPTS + 6);
    *B(GAME + 119) = *B(OPTS + 7);
    uint32_t kind = *B(NETP + 42);
    if (!r20_early) c->r[20] = kind;
    *B(GAME + 156) = *B(OPTS + 8);
    int many = 0;
    if (kind == 3 || kind == 2) {
        call0(c, f_088DD114, back_count);
        many = !((int32_t)c->r[2] < 2);
    }
    if (many) { *B(NETP + 265) = 1; *B(NETP + 266) = 0x18; }
    else { *B(NETP + 265) = 0; *B(NETP + 266) = 0; }
}

/* A transfer's result: an error is reported (0x088D5680); bytes moved come off what is left (+26)
   and with none left the next step with a 0x2D0 timeout; nothing moved counts the timeout (+4)
   down, and at 0 reports 0x81001001. */
static void moved(Cpu *c, gaddr x, uint32_t back_error, uint32_t back_timeout) {
    int32_t got = (int32_t)c->r[2];
    c->r[20] = (uint32_t)got;
    if (got < 0) { call2(c, x, (uint32_t)got, f_088D5680, back_error); return; }
    if (got > 0) {
        *H(x + 26) = (uint16_t)(S16(x + 26) - got);
        if (S16(x + 26) > 0) return;
        *H(x) = (uint16_t)(*H(x) + 1);
        *H(x + 4) = 0x2D0;
        return;
    }
    int32_t t = S16(x + 4) - 1;
    *H(x + 4) = (uint16_t)t;
    if ((int16_t)t == 0) call2(c, x, 0x81001001u, f_088D5680, back_timeout);
}

/* (x): the settings exchange before a link match, by its step (x + 0):
   0, 1  the packet is cleared with a 0x168 timeout and +6 says whether a second round follows (more
         than one peer, kinds 2 and 3); then 0x088D67C4 fills it, it is kept as this side's settings
         (the side is +32 for the link kinds 2 and 4, else 0x088DD124), side 0's options are taken,
         and it is marked 0x12345678 and summed (0x088DC054), with 0xE4 bytes to send;
   2     sent (0x088DF910 on +12);
   3     once 0x088DFC10 says so the packet is cleared for 0xE4 bytes to come;
   4     received (0x088DFA48 on +16);
   5     the sum is checked: a wrong one reports 0x81001002;
   6     in a second round what came is kept as side +16's and step 2 runs again; else it is the
         other side's (not +32 for the kinds 2 and 4, else +12);
   7     waits for 0x088E3F44;
   8     done: +24 goes up and the steps start over.
   Any other step resets (0x088DF778, the handler 0x08A3EB14). It ends by drawing the message for
   +42 + +43 from the table 0x08A3EA04 (0x0880A6CC at 2, 0x30). A 0x50 frame keeping s0..s7, fp. */
SFA_FUNC(088DA91C, link_swap_settings) {
    gaddr x = ARG(0);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 40) = sv[1];
    c->r[17] = x;
    uint32_t step = *H(x);
    *W(sp + 52) = sv[4]; *W(sp + 64) = sv[7];
    c->r[23] = 2; c->r[20] = SWAP;
    *W(sp + 36) = sv[0]; *W(sp + 44) = sv[2]; *W(sp + 48) = sv[3]; *W(sp + 56) = sv[5]; *W(sp + 60) = sv[6];
    *W(sp + 68) = sv[8]; *W(sp + 72) = sv[9];
    c->r[22] = 4;
    switch (step) {
    case 0:
        call3(c, SWAP, 0, 0xE4, f_089F3470, 0x088DA990u);
        *H(x + 4) = 0x168;
        {
            uint32_t kind = *B(x + 42);
            c->r[19] = NETP; c->r[21] = SETS; c->r[18] = 0; c->r[16] = SWAP + 8; c->r[30] = 0xE4;
            *W(sp + 32) = SWAP + 4;
            if (kind == 3 || kind == 2) {
                call0(c, f_088DD114, 0x088DA9D8u);
                if (!((int32_t)c->r[2] < 2)) c->r[18] = 1;
            }
            *H(x + 6) = (uint16_t)c->r[18];
            *H(x) = (uint16_t)(*H(x) + 1);
        }
        goto fill;
    case 1:
        *W(sp + 32) = SWAP + 4;
        c->r[19] = NETP; c->r[21] = SETS; c->r[16] = SWAP + 8; c->r[30] = 0xE4;
    fill:
        call2(c, x, SWAP, f_088D67C4, 0x088DAA28u);
        {
            uint32_t side, kind = *B(NETP + 43);
            if (kind == 2 || kind == 4) side = *B(x + 32);
            else { call0(c, f_088DD124, 0x088DAA4Cu); side = c->r[2]; }
            c->r[21] = SETS + side * 208;
            keep_side(side);
            if (side == 0) {
                c->r[16] = OPTS;
                take_options(c, SWAP, 1, 0x088DAAB4u, 0x088DAB2Cu);
            }
        }
        *W(SWAP + 4) = 0x12345678u;
        call2(c, *W(sp + 32), 0xE0, f_088DC054, 0x088DAB68u);
        *W(SWAP) = c->r[2];
        {
            uint16_t s = *H(x);
            *H(x + 26) = 0xE4;
            *H(x) = (uint16_t)(s + 1);
        }
        /* fall through */
    case 2: {
        uint32_t left = (uint32_t)(int32_t)S16(x + 26);
        c->r[30] = left;
        ARG(3) = 0;
        call3(c, *W(x + 12), SWAP - left + 0xE4, left, f_088DF910, 0x088DABA4u);
        moved(c, x, 0x088DABB8u, 0x088DAC1Cu);
        break;
    }
    case 3:
        call1(c, *W(x + 12), f_088DFC10, 0x088DAC2Cu);
        if (c->r[2] != 0) {
            call3(c, SWAP, 0, 0xE4, f_089F3470, 0x088DAC40u);
            uint16_t s = *H(x);
            *H(x + 26) = 0xE4;
            *H(x) = (uint16_t)(s + 1);
        }
        break;
    case 4: {
        uint32_t left = (uint32_t)(int32_t)S16(x + 26);
        ARG(3) = 0;
        call3(c, *W(x + 16), SWAP - left + 0xE4, left, f_088DFA48, 0x088DAC70u);
        moved(c, x, 0x088DAC84u, 0x088DACE8u);
        break;
    }
    case 5:
        call2(c, SWAP + 4, 0xE0, f_088DC054, 0x088DACFCu);
        if (c->r[2] != *W(SWAP)) call2(c, x, 0x81001002u, f_088D5680, 0x088DAD14u);
        *H(x) = (uint16_t)(*H(x) + 1);
        break;
    case 6:
        if (S16(x + 6) != 0) {
            uint32_t side = *W(x + 16);
            keep_side(side);
            if (side == 0) {
                c->r[16] = OPTS;
                take_options(c, NETP, 1, 0x088DADA0u, 0x088DAE20u);
            }
            *H(x + 26) = 0xE4;
            *H(x + 6) = (uint16_t)(S16(x + 6) - 1);
            *H(x) = 2;
        } else {
            c->r[16] = NETP;
            uint32_t side, kind = *B(NETP + 43);
            if (kind == 2 || kind == 4) side = *B(x + 32) ^ 1u;
            else side = *W(x + 12);
            keep_side(side);
            if (side == 0) {
                c->r[18] = OPTS;
                take_options(c, 0, 0, 0x088DAEECu, 0x088DAF64u);
            }
            *H(x) = (uint16_t)(*H(x) + 1);
        }
        break;
    case 7:
        call0(c, f_088E3F44, 0x088DAFA0u);
        if (c->r[2] != 0) *H(x) = (uint16_t)(*H(x) + 1);
        break;
    case 8:
        {
            uint16_t n = *H(x + 24);
            *H(x + 2) = 0;
            *H(x + 24) = (uint16_t)(n + 1);
            *H(x) = 0;
            *B(x + 47) = 0;
        }
        break;
    default:
        call2(c, step, 0, f_088DF778, 0x088DAFDCu);
        *W(x + 20) = 0x08A3EB14u;
        *H(x + 24) = 0; *H(x + 2) = 0; *H(x) = 0;
        *B(x + 47) = 0;
        break;
    }
    ARG(1) = 2; ARG(2) = 0x30; ARG(3) = 4; c->r[8] = 9;
    call1(c, *W(0x08A3EA04u + (*B(x + 42) + *B(x + 43)) * 4), f_0880A6CC, 0x088DB02Cu);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x50;
}
