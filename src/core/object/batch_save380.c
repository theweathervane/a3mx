/* The step of a save or a load in progress: when the dialog has finished, what was read is put
   where the game keeps it. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE_JOB 0x09523C60u /* the job: +6 its slot, +8 whether it saves, +12 what it loads */
#define READ_BUF 0x08AEF4D8u /* where the dialog reads a file to */
#define OPTIONS  0x08A058A0u /* the game's settings */

void f_089E3C30(Cpu *c); void f_089E3C74(Cpu *c); void f_089E3CE0(Cpu *c); void f_08816FBC(Cpu *c);
void f_089E0DC4(Cpu *c); void f_088B70C0(Cpu *c); void f_089E6898(Cpu *c); void f_089E688C(Cpu *c);
void f_08815478(Cpu *c); void f_088155C0(Cpu *c); void f_08815644(Cpu *c); void f_088051A0(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void copy(Cpu *c, uint32_t to, uint32_t from, uint32_t n, uint32_t back) {
    ARG(1) = from; ARG(2) = n; ARG(0) = to; c->r[31] = back; f_089E0DC4(c);
}

/* The job's step. -1 while the dialog is still up. When its result is an error, the job ends with
   3 and the error is returned; 0xFFFF0000 while the result is still awaited. A save ends with 2.
   A load is first checked (unless it is of kind 3 or 4, the file must have 0x3399 at +640, or the
   job ends with 3 and 0xFFFFFF41 is returned), then by its kind: 0 takes the settings (keeping
   the byte at +71), four more blocks and a word, applies them (the two volumes, the language's
   three routines when +510 changed, and +511) and copies +79 to 0x08A077CC; 1 takes 0x174C bytes
   to 0x09517E38; 4 takes 0x13F8 bytes to 0x0951A980. The job then ends with 2 and 0 is returned.
   A 0x30 frame keeping s0..s5. */
SFA_FUNC(08816FC4, save_job_step) {
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1];
    c->r[17] = SAVE_JOB;
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    *W(sp + 36) = sv[5]; *W(sp + 40) = sv[6];
    uint32_t result;
    call1(c, (uint32_t)S8(SAVE_JOB + 6), f_089E3C30, 0x08816FF4u);
    c->r[16] = c->r[2];
    ARG(0) = 0xFFFFFFFFu;
    result = c->r[16];
    if (c->r[16] == 0xFFFFFFFFu) goto done;
    call1(c, (uint32_t)S8(SAVE_JOB + 6), f_089E3C74, 0x0881700Cu);
    c->r[16] = c->r[2];
    result = c->r[16];
    if (c->r[16] != 0) {
        call2(c, 3, (uint32_t)S8(SAVE_JOB + 6), f_08816FBC, 0x08817234u);
        goto done;
    }
    c->r[31] = 0x08817020u; f_089E3CE0(c);
    if (c->r[2] != 0) { result = 0xFFFF0000u; goto done; }
    ARG(0) = (uint32_t)S8(SAVE_JOB + 8);
    if (ARG(0) != 0) {
        call2(c, 2, (uint32_t)S8(SAVE_JOB + 6), f_08816FBC, 0x08817040u);
        goto done;
    }
    {
        uint32_t kind = (uint32_t)S8(SAVE_JOB + 12);
        ARG(1) = kind;
        ARG(0) = 3;
        ARG(2) = 4;
        if (kind != 3) {
            c->r[7] = *W(READ_BUF + 640);
            ARG(2) = 0x3399;
            if (c->r[7] != 0x3399) {
                call2(c, 3, (uint32_t)S8(SAVE_JOB + 6), f_08816FBC, 0x088170B4u);
                result = 0xFFFFFF41u;
                goto done;
            }
            ARG(2) = 4;
        }
        if (kind == 4) {
            copy(c, 0x0951A980u, READ_BUF + 0x214C, 0x13F8, 0x08817218u);
        } else if (kind == 3) {
            ARG(0) = 1;
        } else if (kind == 1) {
            copy(c, 0x09517E38u, READ_BUF + 0x284, 0x174C, 0x088171F4u);
        } else if (kind != 0) {
            ARG(0) = 0x08AF0000u;
            c->r[18] = 0x08A00000u;
        } else {
            c->r[18] = OPTIONS;
            c->r[19] = (uint32_t)S8(OPTIONS + 71);
            c->r[20] = (uint32_t)S8(OPTIONS + 510);
            c->r[21] = READ_BUF;
            copy(c, OPTIONS, READ_BUF + 0x284, 0x208, 0x08817120u);
            *B(OPTIONS + 71) = (uint8_t)c->r[19];
            copy(c, 0x08A05AA8u, READ_BUF + 0x48C, 0x17C0, 0x08817138u);
            copy(c, 0x08A07268u, READ_BUF + 0x1C4C, 0x3C0, 0x0881714Cu);
            copy(c, 0x08A07628u, READ_BUF + 0x200C, 0x140, 0x08817160u);
            copy(c, 0x09519588u, READ_BUF + 0x214C, 0x13F8, 0x08817174u);
            copy(c, 0x08A05894u, READ_BUF + 0x3544, 4, 0x08817188u);
            c->r[31] = 0x08817190u; f_088B70C0(c);
            call1(c, *B(OPTIONS + 508), f_089E6898, 0x08817198u);
            call1(c, *B(OPTIONS + 509), f_089E688C, 0x088171A0u);
            ARG(0) = *B(OPTIONS + 510);
            if (c->r[20] != ARG(0)) {
                c->r[31] = 0x088171B4u; f_08815478(c);
                c->r[31] = 0x088171BCu; f_088155C0(c);
                c->r[31] = 0x088171C4u; f_08815644(c);
            }
            call1(c, *B(OPTIONS + 511), f_088051A0, 0x088171CCu);
            ARG(0) = (uint32_t)S8(OPTIONS + 79);
            ARG(1) = 0x08A00000u;
            *B(0x08A077CCu) = (uint8_t)ARG(0);
        }
        call2(c, 2, (uint32_t)S8(SAVE_JOB + 6), f_08816FBC, 0x08817224u);
    }
done:
    c->r[2] = result;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}
