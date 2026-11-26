/* Small functions the scenarios reach, 129 - 131 instructions: a menu page's reset with its list-0x7E
   objects, the VS screen's start with the chosen character's music. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME  0x08E75910u
#define PICS  0x08E75AD8u
#define TILES 0x092D9D40u
#define PALS  0x092D9E00u

void f_0880AEA8(Cpu *c); void f_08822F1C(Cpu *c); void f_088AC3EC(Cpu *c); void f_088054F0(Cpu *c);
void f_088075F8(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E0C24(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (page): counted (+16), 0x088AC3EC, its cursors (0x08B027C8 / 0x08B027F8 / 0x08B02828) and +17 / +78 -
   +80 / GAME +8 cleared, +76 = 1; with 0x095176F5 at 11 or more list-0x7E objects (0x08822F1C): 5, 0 / 1
   (+78 = 0 / 6, or 0xC / 0x12 with +6 set), three 2s (+78 = 2 - 0) and four 3s (+10 = 3 - 0, +78 - +81
   the bytes of 0x04030F10 turning round); all on; +77 = 4, two palettes (0x08A32BF8 into 0,
   0x08A32A38 into 0x1E). */
SFA_FUNC(088AB4B8, menu_page_reset) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    *W(sp + 36) = sv[1];
    c->r[17] = 0;
    for (int i = 2; i < 6; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 56) = ra;
    *B(o + 16) = (uint8_t)(*B(o + 16) + 1);
    call_on(c, o, 0x088AB4F0u, f_088AC3EC);
    *W(0x08B02828u + 4) = 0;
    static const uint32_t cur[3] = { 0x08B02828u, 0x08B027F8u, 0x08B027C8u };
    for (int i = 0; i < 3; i++) { *H(cur[i] + 18) = 0; *H(cur[i] + 16) = 0; }
    *W(0x08B02828u) = 0;
    for (int i = 1; i < 3; i++) { *W(cur[i] + 4) = 0; *W(cur[i]) = 0; }
    *B(o + 80) = 0; *B(o + 79) = 0; *B(o + 78) = 0;
    *B(o + 17) = 0;
    *H(GAME + 8) = 0;
    c->r[18] = 1;
    *B(o + 76) = 1;
    if (S8(0x095176F5u) >= 11) {
        uint32_t a = 0, b = 6;
        if (*H(o + 6) != 0) { a = 0xC; b = 0x12; }
        c->r[19] = a;
        call2(c, 0x7E, 5, 0x088AB59Cu, f_08822F1C);
        *B(c->r[2]) = 1;
        call2(c, 0x7E, 0, 0x088AB5ACu, f_08822F1C);
        *B(c->r[2]) = 1; *B(c->r[2] + 78) = (uint8_t)a;
        call2(c, 0x7E, 1, 0x088AB5C0u, f_08822F1C);
        *B(c->r[2] + 78) = (uint8_t)b; *B(c->r[2]) = 1;
        for (int32_t j = 2; j >= 0; j--) {
            c->r[17] = (uint32_t)j;
            call2(c, 0x7E, 2, 0x088AB5D8u, f_08822F1C);
            *B(c->r[2] + 78) = (uint8_t)j;
            *B(c->r[2]) = 1;
        }
        uint32_t w = 0x04030F10u;
        c->r[20] = 0xFF000000u; c->r[19] = 0xFF0000u;
        for (int32_t j = 3; j >= 0; j--) {
            c->r[17] = (uint32_t)j; c->r[21] = w;
            call2(c, 0x7E, 3, 0x088AB608u, f_08822F1C);
            gaddr n = c->r[2];
            *B(n + 10) = (uint8_t)j;
            *B(n) = 1;
            *B(n + 78) = (uint8_t)((w & 0xFF000000u) >> 24);
            *B(n + 79) = (uint8_t)((int32_t)(w & 0xFF0000u) >> 16);
            *B(n + 80) = (uint8_t)((int32_t)(w & 0xFF00u) >> 8);
            *B(n + 81) = (uint8_t)(w & 0xFF);
            w = ((w & 0xFF) << 24) + (uint32_t)((int32_t)w >> 8);
        }
        c->r[17] = (uint32_t)-1;
    }
    *B(o + 77) = 4;
    ARG(0) = 1; ARG(1) = 0x08A32BF8u; ARG(2) = 0; ARG(3) = 0; c->r[8] = 1;
    c->r[31] = 0x088AB678u; f_0880AEA8(c);
    ARG(0) = 1; ARG(1) = 0x08A32A38u; ARG(2) = 0; ARG(3) = 0x1E; c->r[8] = 1;
    c->r[31] = 0x088AB698u; f_0880AEA8(c);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* The VS screen's start (between 0x088D7EA0 (1) and (0)): which of the three 0x2F8-byte records at
   0x08A05AA8 has a set +15 among its 34 20-byte entries goes to 0x09357C00, and that record's +8 is the
   character; with 0x08E75C54 set the textures 0xE0 - 0xE5 are freed (0x089E0C24), then it is set; the
   sound slot's flags cleared, the pictures / tiles / palette buffers handed over (0x08A0562C /
   0x08A056D4 / 0x08A0577C +88 / +92 into PICS / TILES / PALS), 0x088075F8 for 0x08A03140 +72 and the
   character's music (0x08A03140 +384, 0x13 -> 0x11, 0x25 -> 0x15), then a wait (0x088054F0 (1)) for
   0x08E76068 +4. */
SFA_FUNC(08814B88, vs_screen_start) {
    uint32_t ra = c->r[31];
    uint32_t sv[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 7; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 44) = ra;
    call_on(c, 1, 0x08814BB4u, f_088D7EA0);
    const gaddr recs = 0x08A05AA8u;
    c->r[17] = recs; c->r[18] = 0x08E70000u; c->r[19] = 0x08A03140u; c->r[20] = 0x08E76068u;
    uint32_t h = *H(0x08E75C54u);
    for (uint32_t i = 0; i < 3; i++) {
        gaddr e = recs + 0x2F8 * i;
        for (int j = 0; j < 34; j++, e += 20)
            if (S8(e + 15) != 0) { *B(0x09357C00u) = (uint8_t)i; break; }
    }
    int32_t k = S8(0x09357C00u);
    uint32_t ch = *B(recs + 0x2F8u * (uint32_t)k + 8);
    c->r[17] = ch;
    if (h != 0)
        for (uint32_t i = 0; i < 6; i++) { c->r[16] = i; call_on(c, i + 0xE0, 0x08814C54u, f_089E0C24); }
    *H(0x08E75C54u) = 1;
    uint32_t s = *B(0x08E76084u);
    gaddr e = 216u * s + 0x08E76088u;
    *B(e + 202) = 0; *B(e + 203) = 0;
    *H(e + 208) = 0;
    *B(e + 204) = 0;
    c->r[16] = 0x08A0562Cu; c->r[18] = PICS; c->r[21] = 0x08A056D4u; c->r[22] = TILES;
    *W(PICS + 88) = *W(0x08A0562Cu + 88);
    *W(TILES + 88) = *W(0x08A056D4u + 88);
    ARG(0) = *W(0x08A03140u + 72); ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08814CE4u; f_088075F8(c);
    *W(PICS + 92) = *W(0x08A0562Cu + 92);
    *W(TILES + 92) = *W(0x08A056D4u + 92);
    *W(PALS + 92) = *W(0x08A0577Cu + 92);
    if (ch == 0x13) ch = 0x11;
    else if (ch == 0x25) ch = 0x15;
    c->r[17] = ch;
    ARG(0) = *W(0x08A03140u + 384 + (ch << 2)); ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08814D44u; f_088075F8(c);
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08814D58u, f_088054F0);
    call_on(c, 0, 0x08814D6Cu, f_088D7EA0);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
