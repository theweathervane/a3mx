/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 103 -
   109 instructions: the select screen's list 18 buffers made and loaded, a list's buffers freed, the
   VS screen's stage background loaded. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088054F0(Cpu *c); void f_088075F8(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E04EC(Cpu *c);
void f_089E0C24(Cpu *c); void f_089E0540(Cpu *c); void f_089E157C(Cpu *c); void f_0880AEA8(Cpu *c);
void f_08813770(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

#define PICS  0x08E75AD8u   /* each list's picture list */
#define TILES 0x092D9D40u   /* its tile set */
#define PALS  0x092D9E00u   /* its palettes */
#define KEPT  0x08A055DCu   /* the buffers the select screen made */

static void record_clear(void) {
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
}

/* Under the loader's lock (0x088D7EA0 (1) / (0)), unless list 18 already has its pictures: textures
   0xE0 - 0xE5 freed if 0x08E75C54 was set (then set), the current 216-byte record's +202 - +204 / +208
   cleared, three buffers made (0x089E04EC: 0x2000, 0x16C00, 0x800) as list 18's pictures, tiles and
   palettes (and kept at 0x08A055F8 - 0x08A05600), its tile list put back (0x08A050B0), 0x088075F8
   (0x08A03198's word, 0, 0) and a wait (0x088054F0 (1) a frame) until 0x08E7606C is set. */
SFA_FUNC(08814ED8, select_list18_load) {
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    for (int k = 0; k < 6; k++) *W(c->r[29] + 16 + 4u * (uint32_t)k) = sv[k];
    *W(c->r[29] + 40) = ra;
    call_on(c, 1, 0x08814F00u, f_088D7EA0);
    c->r[19] = PICS; c->r[18] = 0x08E70000u;
    if (*W(PICS + 72) == 0) {
        c->r[16] = 0x08E76068u;
        if (*H(0x08E75C54u) != 0)
            for (c->r[17] = 0; (int32_t)c->r[17] < 6; c->r[17]++) call_on(c, c->r[17] + 0xE0, 0x08814F30u, f_089E0C24);
        c->r[17] = 0x08E70000u;
        *H(0x08E75C54u) = 1;
        c->r[18] = 0x08E76088u;
        record_clear();
        call_on(c, 0x2000, 0x08814F84u, f_089E04EC); c->r[20] = c->r[2];
        call_on(c, 0x16C00, 0x08814F94u, f_089E04EC); c->r[21] = c->r[2];
        call_on(c, 0x800, 0x08814FA0u, f_089E04EC);
        *W(KEPT + 28) = c->r[20]; *W(PICS + 72) = c->r[20];
        *W(KEPT + 32) = c->r[21]; *W(TILES + 72) = c->r[21];
        *W(KEPT + 36) = c->r[2]; *W(PALS + 72) = c->r[2];
        record_clear();
        uint32_t arc = *W(0x08A03140u + 88);
        *W(0x08E75A18u + 72) = *W(0x08A050B0u + 72);
        call3(c, arc, 0, 0, 0x08815028u, f_088075F8);
        while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x0881503Cu, f_088054F0);
    }
    call_on(c, 0, 0x08815050u, f_088D7EA0);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* The three buffers kept at `kept` (+0 / +4 / +8) freed (0x089E0540) and taken off list k. */
static void free_kept(Cpu *c, uint32_t k, gaddr kept, const uint32_t backs[3]) {
    static const gaddr lists[3] = { PICS, TILES, PALS };
    c->r[19] = KEPT;
    uint32_t a = *W(kept), b = *W(kept + 4), d = *W(kept + 8);
    c->r[18] = b; c->r[17] = d;
    const uint32_t bufs[3] = { a, b, d };
    for (int n = 0; n < 3; n++) {
        if (bufs[n] == 0) continue;
        call_on(c, bufs[n], backs[n], f_089E0540);
        *W(lists[n] + (k << 2)) = 0;
        *W(kept + 4u * (uint32_t)n) = 0;
    }
}

/* (list): its buffers freed: list 13 and 18 the ones the select screen made (18 also textures 0x30 -
   0x34), any other its tile list (0x089E157C). */
SFA_FUNC(08812DDC, list_free) {
    uint32_t k = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = ra;
    c->r[16] = k;
    if (k == 0xD) {
        static const uint32_t b[3] = { 0x08812E20u, 0x08812E48u, 0x08812E70u };
        free_kept(c, k, KEPT + 4, b);
    } else if (k == 0x12) {
        static const uint32_t b[3] = { 0x08812EB8u, 0x08812EE0u, 0x08812F08u };
        free_kept(c, k, KEPT + 28, b);
        static const uint32_t tb[5] = { 0x08812F28u, 0x08812F30u, 0x08812F38u, 0x08812F40u, 0x08812F48u };
        for (int n = 0; n < 5; n++) call_on(c, 0x30u + (uint32_t)n, tb[n], f_089E0C24);
    } else {
        gaddr p = 0x08E75A18u + (k << 2);
        c->r[16] = p;
        if (*W(p) != 0) { call_on(c, *W(p), 0x08812F70u, f_089E157C); *W(p) = 0; }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (stage, wait): the VS screen's stage background, under the loader's lock, unless it is already the
   one shown (0x08E75C50): 0x092E89F8 = stage, 0x08813770, list 4 / 15's pictures, tiles and palettes
   (0x08A051B4 as list 4's tiles for stages flagged at 0x08A05158), 0x08A051B8 / 0x08A053D0 from the
   stage's bytes, the current record cleared, 0x08A055DC = 0x092E8A00, 0x088075F8 (the stage's archive
   from 0x08A03200, 0, 0); with `wait`, a wait for the loader, the 6 palettes into slot 0x38 and
   0x08E75C50 set. */
SFA_FUNC(08813818, vs_stage_load) {
    uint32_t k = ARG(0), wait = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20];
    c->r[29] -= 0x40;
    *W(c->r[29] + 44) = s3; *W(c->r[29] + 48) = s4; *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1;
    *W(c->r[29] + 40) = s2; *W(c->r[29] + 52) = ra;
    c->r[20] = k; c->r[19] = wait;
    call_on(c, 1, 0x08813844u, f_088D7EA0);
    c->r[18] = 0x08E70000u; c->r[17] = 0x092F0000u;
    uint32_t shown = *H(0x08E75C50u);
    if (shown != k) {
        *H(0x092E89F8u) = (uint16_t)k;
        call_on(c, shown, 0x0881385Cu, f_08813770);
        uint32_t flag = *B(0x08A05158u + k);
        c->r[16] = 0x08A0577Cu;
        if (flag != 0) *W(0x08E75A18u + 16) = 0x08A051B4u;
        *W(PALS + 60) = *W(0x08A0577Cu + 60);
        *W(0x08A051B4u + 4) = flag;
        *W(0x08A053CCu + 4) = *B(0x08A051BCu + k);
        record_clear();
        uint32_t arc = *W(0x08A03140u + 192 + (k << 2));
        *W(PICS + 16) = *W(0x08A0562Cu + 16);
        *W(PICS + 60) = *W(0x08A0562Cu + 60);
        *W(TILES + 60) = *W(0x08A056D4u + 60);
        *W(KEPT) = 0x092E8A00u;
        call3(c, arc, 0, 0, 0x08813958u, f_088075F8);
        c->r[19] = 0x08E70000u;
        if (wait != 0) {
            c->r[19] = 0x08E76068u;
            while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08813978u, f_088054F0);
            ARG(0) = 1; ARG(1) = *W(0x08A0577Cu + 60); ARG(2) = 0; ARG(3) = 0x38; c->r[8] = 6;
            c->r[31] = 0x0881399Cu; f_0880AEA8(c);
            *H(0x08E75C50u) = *H(0x092E89F8u);
        }
    }
    call_on(c, 0, 0x088139ACu, f_088D7EA0);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x40;
}
