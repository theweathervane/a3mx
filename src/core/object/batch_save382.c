/* The two save jobs the game runs by itself, each keeping the frames going while the system's
   dialog works: the settings loaded when the game starts, and saved when they change. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define AUTOSAVE_ON 0x08A077CCu /* whether the game saves and loads by itself */
#define CARD_NAMES  0x08A07768u /* the names the files are looked up by, 16 bytes each */
#define FILE_SIZES  0x08A077B8u /* two sizes, in blocks of 512 bytes */

void f_089E3D08(Cpu *c); void f_089E37EC(Cpu *c); void f_0888BCC8(Cpu *c); void f_08837690(Cpu *c);
void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c); void f_088054F0(Cpu *c); void f_089E32CC(Cpu *c);
void f_089E3838(Cpu *c); void f_089E352C(Cpu *c); void f_089E358C(Cpu *c); void f_08816FBC(Cpu *c);
void f_088168E0(Cpu *c); void f_08816FC4(Cpu *c); void f_089E39B8(Cpu *c); void f_08816A78(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* Frames are run (the first return address is at `at`, the rest 8 apart) until `step` answers
   something other than -1. `first` says whether the frame begins with the extra routine. */
static uint32_t frames_until(Cpu *c, uint32_t at, int first, void (*step)(Cpu *)) {
    do {
        uint32_t back = at;
        if (first) { c->r[31] = back; f_0888BCC8(c); back += 8; }
        c->r[31] = back; f_08837690(c);
        c->r[31] = back + 8; f_08887AFC(c);
        c->r[31] = back + 16; f_0880B6CC(c);
        ARG(0) = 1; c->r[31] = back + 24; f_088054F0(c);
        c->r[31] = back + 32; step(c);
    } while (c->r[2] == 0xFFFFFFFFu);
    return c->r[2];
}

/* The settings loaded when the game starts. 4 when the game does not load by itself; 0xA when
   the dialog cannot start, the check or the free space fails; 0x10 when there is no data (-251);
   then the load: 5 when it worked (the settings' +79 becomes the flag), 0x14 for the answer -245,
   6 for any other error. A 0x20 frame keeping s0, s1. */
SFA_FUNC(0881725C, settings_autoload) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = 0x08A00000u;
    ARG(0) = *B(AUTOSAVE_ON);
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    uint32_t answer;
    do {
        if (ARG(0) == 0) { answer = 4; break; }
        ARG(0) = 0; c->r[31] = 0x08817284u; f_089E3D08(c);
        ARG(0) = 0x08A00000u;
        answer = 0xA;
        if (c->r[2] == 0) break;
        ARG(0) = CARD_NAMES; c->r[31] = 0x08817294u; f_089E37EC(c);
        c->r[16] = 0xFFFFFFFFu;
        frames_until(c, 0x088172D4u, 0, f_089E32CC);
        c->r[31] = 0x08817304u; f_089E3838(c);
        int32_t r = (int32_t)c->r[2];
        ARG(0) = (uint32_t)r;
        ARG(1) = r < -250;
        if (r < -250) {
            ARG(0) = r < -251;
            if (r == -251) answer = 0x10;
            break;
        }
        if (r != 0) break;
        c->r[31] = 0x08817368u; f_089E352C(c);
        frames_until(c, 0x08817370u, 0, f_089E32CC);
        c->r[31] = 0x088173A0u; f_089E358C(c);
        ARG(0) = 1;
        if ((int32_t)c->r[2] < 0) break;
        call2(c, 1, 0, f_08816FBC, 0x088173B0u);
        call2(c, 0, 0, f_088168E0, 0x088173BCu);
        r = (int32_t)frames_until(c, 0x088173E4u, 0, f_08816FC4);
        ARG(0) = (uint32_t)r;
        ARG(1) = r < -244;
        if (r < -244) {
            ARG(0) = r < -245;
            if (r == -245) { answer = 0x14; break; }
        } else if (r == 0) {
            call2(c, 2, 0, f_08816FBC, 0x0881745Cu);
            ARG(0) = (uint32_t)S8(0x08A058A0u + 79);
            *B(AUTOSAVE_ON) = (uint8_t)ARG(0);
            answer = 5;
            break;
        }
        call2(c, 3, 0, f_08816FBC, 0x08817430u);
        answer = 6;
    } while (0);
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The settings saved by the game itself. 9 when the dialog cannot start or the free space cannot
   be read. Both files are looked for, then the free space: when the first is there already the
   save goes ahead (1 when it worked, 2 when not); otherwise the space is compared with what is
   needed (the first size, or both when the second file is missing too): 0xB or 0xC when there is
   too little, 0x10 or 0x11 when there is enough. A 0x30 frame keeping s0..s4. */
SFA_FUNC(08816BBC, settings_autosave) {
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[2];
    c->r[18] = 1;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    *W(sp + 36) = sv[5];
    uint32_t answer;
    do {
        call2(c, 1, 0, f_08816FBC, 0x08816BE8u);
        ARG(0) = 0; c->r[31] = 0x08816BF0u; f_089E3D08(c);
        c->r[16] = 0x08A00000u;
        answer = 9;
        if (c->r[2] == 0) break;
        c->r[16] = CARD_NAMES;
        ARG(0) = CARD_NAMES; c->r[31] = 0x08816C04u; f_089E37EC(c);
        c->r[17] = 0xFFFFFFFFu;
        frames_until(c, 0x08816C38u, 1, f_089E32CC);
        c->r[31] = 0x08816C70u; f_089E3838(c);
        uint32_t first = c->r[2];
        c->r[20] = first;
        ARG(0) = CARD_NAMES + 0x10; c->r[31] = 0x08816C7Cu; f_089E37EC(c);
        frames_until(c, 0x08816C84u, 1, f_089E32CC);
        c->r[31] = 0x08816CBCu; f_089E3838(c);
        uint32_t second = c->r[2];
        c->r[19] = second;
        c->r[31] = 0x08816CC4u; f_089E352C(c);
        frames_until(c, 0x08816CCCu, 1, f_089E32CC);
        c->r[31] = 0x08816D04u; f_089E358C(c);
        int32_t space = (int32_t)c->r[2];
        c->r[16] = (uint32_t)space;
        if (space < 0) break;
        ARG(0) = 0;
        if (first == 0) {
            call2(c, 0, 0, f_08816A78, 0x08816DE0u);
            c->r[16] = 2;
            uint32_t r = frames_until(c, 0x08816DECu, 1, f_08816FC4);
            if (r == 0) { call2(c, 2, 0, f_08816FBC, 0x08816E60u); answer = 1; }
            else { call2(c, 3, 0, f_08816FBC, 0x08816E34u); answer = 2; }
            break;
        }
        c->r[17] = 0x08A00000u;
        ARG(0) = *W(FILE_SIZES) << 9;
        if (second == 0) {
            c->r[31] = 0x08816DA4u; f_089E39B8(c);
            ARG(0) = space < (int32_t)c->r[2];
            answer = ARG(0) ? 0xB : 0x10;
        } else {
            c->r[31] = 0x08816D54u; f_089E39B8(c);
            c->r[17] = c->r[2];
            ARG(0) = *W(FILE_SIZES + 4) << 9;
            c->r[31] = 0x08816D68u; f_089E39B8(c);
            ARG(0) = space < (int32_t)(c->r[17] + c->r[2]);
            answer = ARG(0) ? 0xC : 0x11;
        }
    } while (0);
    c->r[2] = answer;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}
