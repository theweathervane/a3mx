/* What the small ones set free: steps that pass on by kind, strength or chance, a menu's cursor
   and choice, and the combo and time texts. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

void f_0882C384(Cpu *c); void f_0882C410(Cpu *c); void f_0882C4D0(Cpu *c); void f_0882C558(Cpu *c);
void f_0887C4CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c);
void f_088D2B5C(Cpu *c); void f_08832728(Cpu *c); void f_088D7C4C(Cpu *c); void f_0883296C(Cpu *c);
void f_08847E80(Cpu *c); void f_08847F2C(Cpu *c); void f_08847F90(Cpu *c); void f_08847FE4(Cpu *c);
void f_08848050(Cpu *c); void f_088215C4(Cpu *c); void f_08880938(Cpu *c); void f_08880898(Cpu *c);
void f_088807D8(Cpu *c); void f_08909110(Cpu *c); void f_089091D8(Cpu *c); void f_089093BC(Cpu *c);
void f_08910D94(Cpu *c); void f_089110B0(Cpu *c); void f_08910D44(Cpu *c); void f_08925760(Cpu *c);
void f_0892582C(Cpu *c); void f_08925A84(Cpu *c); void f_08925AFC(Cpu *c); void f_0892A258(Cpu *c);
void f_0892A15C(Cpu *c); void f_0886D678(Cpu *c); void f_0892B188(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c); void f_08938D40(Cpu *c); void f_08864740(Cpu *c);
void f_08938A5C(Cpu *c); void f_08938998(Cpu *c); void f_08861848(Cpu *c); void f_08943028(Cpu *c);
void f_08942D54(Cpu *c); void f_08942C80(Cpu *c); void f_08942894(Cpu *c); void f_08966B54(Cpu *c);
void f_08966EF8(Cpu *c); void f_08966C60(Cpu *c); void f_08967024(Cpu *c); void f_0896709C(Cpu *c);
void f_0896BA88(Cpu *c); void f_0896B800(Cpu *c); void f_08821368(Cpu *c); void f_089897BC(Cpu *c);
void f_0898958C(Cpu *c); void f_08989878(Cpu *c); void f_088D3558(Cpu *c); void f_088D36F8(Cpu *c);
void f_088D3EC0(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
/* A 0x20 frame that keeps only ra, at +16. */
static uint32_t enter0(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave0(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }

/* (x, y): a grid menu for the side x's +53 says (its pad at 0x08E75BA0, or 0x2C further on). A
   direction just pressed moves the cursor (left, up, right, down: 0x0882C384, 0x0882C410,
   0x0882C4D0, 0x0882C558 with y) and waits 0x14 frames at 0x08B01F35 before repeating; one held
   repeats every 6. A confirm button (0x8080) moves x's +4 on with the sound 0x39, and by the mode
   (y + 79) keeps the fight's kind at y + 56: 3 for the modes 5 and 4, the side (1 or 2) for the
   mode 6. The button 0x100 clears the mode and moves on too. Then the text 0x089FE0D0 and the
   three things every menu frame does. A 0x40 frame keeping s0..s3. */
SFA_FUNC(0882C0A4, grid_menu) {
    static void (*const move[4])(Cpu *) = { f_0882C384, f_0882C410, f_0882C4D0, f_0882C558 };
    static const uint32_t bit[4] = { 4, 1, 8, 2 };
    static const uint32_t back_new[4] = { 0x0882C110u, 0x0882C138u, 0x0882C160u, 0x0882C184u };
    static const uint32_t back_held[4] = { 0x0882C1C8u, 0x0882C208u, 0x0882C248u, 0x0882C284u };
    const gaddr wait = 0x08B01F35u;
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = sv[2]; *W(sp + 32) = sv[0]; *W(sp + 44) = sv[3]; *W(sp + 36) = sv[1]; *W(sp + 48) = sv[4];
    gaddr pad = 0x08E75BA0u;
    if ((S8(x + 53) & 1) == 0) pad += 0x2C;
    c->r[19] = 5; c->r[16] = x; c->r[18] = pad; c->r[17] = y;
    uint32_t pressed = *W(pad + 12);
    int i;
    for (i = 0; i < 4; i++) {
        if (pressed & bit[i]) {
            *B(wait) = 0x14;
            call1(c, y, move[i], back_new[i]);
            break;
        }
    }
    if (i == 4) {
        uint32_t held = *W(pad + 4);
        for (i = 0; i < 4; i++) {
            if (held & bit[i]) {
                int32_t t = S8(wait) - 1;
                *B(wait) = (uint8_t)t;
                if ((int8_t)t < 0) {
                    *B(wait) = 6;
                    call1(c, y, move[i], back_held[i]);
                }
                break;
            }
        }
    }
    uint32_t buttons = *W(pad + 12);
    c->r[18] = buttons;
    if (buttons & 0x8080) {
        *H(x + 4) = (uint16_t)(*H(x + 4) + 1);
        call3(c, 0, 0x39, 0xC0, f_0887C4CC, 0x0882C2B0u);
        int32_t mode = S8(y + 79);
        if (mode == 5 || mode == 4) *H(y + 56) = 3;
        else if (mode == 6) {
            int32_t side = S8(x + 53);
            c->r[16] = (uint32_t)side;
            if (side & 1) *H(y + 56) = 1;
            else if (side & 2) *H(y + 56) = 2;
        }
    } else if (buttons & 0x100) {
        *B(y + 79) = 0;
        *H(x + 4) = (uint16_t)(*H(x + 4) + 1);
        call3(c, 0, 0x39, 0xC0, f_0887C4CC, 0x0882C334u);
    }
    ARG(1) = 2; ARG(2) = 0x58; ARG(3) = 5; c->r[8] = 0xB;
    call1(c, 0x089FE0D0u, f_0880A6CC, 0x0882C350u);
    call0(c, f_08887AFC, 0x0882C358u);
    call0(c, f_0880B6CC, 0x0882C360u);
    call0(c, f_088D2B5C, 0x0882C368u);
    for (int k = 0; k < 4; k++) c->r[16 + k] = sv[k];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* (x, y): in the fight proper (x's first three words 1, 2, 3) both fighters' +412 says whether the
   combo texts show: not while y's +36 is set, else x's +89 plus one (1 for none); and unless y's
   +23 is set the texts are drawn (0x08832728) at 0x20 / 0x40 and 0x100 / 0x108. */
SFA_FUNC(08832678, combo_texts) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[2];
    enter(c, FIGHTER_2, sv);
    if (*H(x) == 1 && *H(x + 2) == 2 && *H(x + 4) == 3) {
        int32_t v = 0;
        if (S8(y + 36) == 0) {
            int32_t t = S8(x + 89);
            v = t == 0 ? 1 : (int8_t)(t + 1);
        }
        *B(FIGHTER_2 + 412) = (uint8_t)v;
        *B(FIGHTER_1 + 412) = (uint8_t)v;
        if (S8(y + 23) == 0) {
            call3(c, FIGHTER_1, 0x20, 0x40, f_08832728, 0x08832708u);
            call3(c, FIGHTER_2, 0x100, 0x108, f_08832728, 0x08832718u);
        }
    }
    leave(c, sv);
}

/* The play time on screen: unless BATTLE + 36 is set, and when 0x088D7C4C says so, the time at
   GAME + 141..143 is written out at 0x08A0F7F5 (0x0883296C) and drawn at 0x128 / 0xB8. A 0x30
   frame keeping s0 at +32. */
SFA_FUNC(08832BB0, time_on_screen) {
    const gaddr text = 0x08A0F7F5u;
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    int32_t off = S8(BATTLE + 36);
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    if (off == 0) {
        call1(c, 0, f_088D7C4C, 0x08832BD8u);
        if (c->r[2] != 0) {
            c->r[16] = text;
            ARG(1) = (uint32_t)(int32_t)S8(GAME + 141);
            ARG(2) = (uint32_t)(int32_t)S8(GAME + 142);
            ARG(3) = (uint32_t)(int32_t)S8(GAME + 143);
            call1(c, text, f_0883296C, 0x08832C00u);
            ARG(1) = 1; ARG(2) = 0x128; ARG(3) = 0xB8; c->r[8] = 3;
            call1(c, text, f_0880A6CC, 0x08832C18u);
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (object): one of five marks by its +3, then drawn (0x088215C4) while it is shown. */
SFA_FUNC(08847DC8, mark_by_kind) {
    static void (*const kind[5])(Cpu *) = { f_08847E80, f_08847F2C, f_08847F90, f_08847FE4, f_08848050 };
    static const uint32_t back[5] = { 0x08847DECu, 0x08847E08u, 0x08847E24u, 0x08847E40u, 0x08847E5Cu };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    uint32_t k = *B(o + 3);
    if (k < 5) call1(c, o, kind[k], back[k]);
    if (*B(o + 1) != 0) call1(c, o, f_088215C4, 0x08847E70u);
    leave(c, sv);
}

/* (object): by its +78: 6 0x08880938, 4 0x08880898, the others 0x088807D8. */
SFA_FUNC(0888077C, object_by_78) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    int32_t k = S8(o + 78);
    c->r[5] = (uint32_t)k;
    if (k == 6) call0(c, f_08880938, 0x0888079Cu);
    else if (k == 4) call0(c, f_08880898, 0x088807B8u);
    else call0(c, f_088807D8, 0x088807CCu);
    leave0(c, ra);
}

/* (fighter): a move by +192 and +193: with +192 0x089093BC; else standing (+696), with +193
   0x089091D8 and without 0x08909110. */
SFA_FUNC(089090B4, move_by_192_090b4) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    if (S8(o + 192) != 0) call0(c, f_089093BC, 0x08909104u);
    else {
        int32_t alt = S8(o + 193);
        *B(o + 696) = 0;
        if (alt != 0) call0(c, f_089091D8, 0x089090F0u);
        else call0(c, f_08909110, 0x089090DCu);
    }
    leave0(c, ra);
}

/* (fighter): the cpu goes by distance (0x08910D44); a player holding any kick 0x089110B0, else
   0x08910D94. */
SFA_FUNC(08910CE4, by_kick_or_distance) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    if (S8(o + 373) != 0) call0(c, f_08910D44, 0x08910D38u);
    else if (*H(o + 760) & 0x380) call0(c, f_089110B0, 0x08910D24u);
    else call0(c, f_08910D94, 0x08910D10u);
    leave0(c, ra);
}

/* (fighter): a move by +192, +193 and its step: with +192, crouching (+696), 0x08925A84 then
   0x08925AFC; else standing: with +193 the strength's own from the table 0x08A6A374, without it
   0x08925760 then 0x0892582C. */
SFA_FUNC(089256AC, move_by_192_256ac) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    if (S8(o + 192) != 0) {
        uint8_t step = *B(o + 7);
        *B(o + 696) = 1;
        if (step != 0) call0(c, f_08925AFC, 0x08925754u);
        else call0(c, f_08925A84, 0x08925740u);
    } else {
        int32_t alt = S8(o + 193);
        *B(o + 696) = 0;
        if (alt != 0) {
            c->r[31] = 0x08925720u;
            rt_call_indirect(c, *W(0x08A6A374u + (uint32_t)(S8(o + 194) << 2)));
        } else if (*B(o + 7) != 0) call0(c, f_0892582C, 0x089256F4u);
        else call0(c, f_08925760, 0x089256E0u);
    }
    leave0(c, ra);
}

/* (fighter): a move by +192 and +193: with +192 0x0892A258; else standing: with +193 0x0892A15C,
   without it two steps: the start (facing as +697 says, +235 set, 0x0892B188, 0x08863114,
   0x0886CA2C with the strength, the animation twice the strength), then 0x0886D678. A frame
   keeping s0, s1. */
SFA_FUNC(0892A0A0, move_by_192_2a0a0) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = o;
    if (S8(o + 192) != 0) call1(c, o, f_0892A258, 0x0892A0C4u);
    else {
        int32_t alt = S8(o + 193);
        *B(o + 696) = 0;
        if (alt != 0) call1(c, o, f_0892A15C, 0x0892A0E0u);
        else if (*B(o + 7) != 0) call1(c, o, f_0886D678, 0x0892A0FCu);
        else {
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 7) = 1;
            *B(o + 11) = (uint8_t)facing;
            *B(o + 235) = 1;
            call1(c, o, f_0892B188, 0x0892A120u);
            call1(c, o, f_08863114, 0x0892A128u);
            int32_t strength = S8(o + 194);
            c->r[17] = (uint32_t)strength;
            call2(c, o, (uint32_t)strength, f_0886CA2C, 0x0892A138u);
            call2(c, o, (uint32_t)(strength << 1) & 0xFFFF, f_08861674, 0x0892A148u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a slide's wait: with no cue (+77) and +0x3C0 clear 0x08938D40 looks first. Ticked to
   the animation's end; then by +0x3C0: 0 ends (+262 cleared, 0x08864740), below 0 0x08938A5C, above
   it the slide (0x08938998). */
SFA_FUNC(089388FC, slide_wait) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 77) == 0 && S8(o + 0x3A0 + 32) == 0) call1(c, o, f_08938D40, 0x0893892Cu);
    if (!(S8(o + 75) < 0)) call1(c, o, f_08861848, 0x08938988u);
    else {
        int32_t v = S8(o + 0x3A0 + 32);
        if (v == 0) {
            *B(o + 262) = 0;
            call1(c, o, f_08864740, 0x08938950u);
        } else if (v < 0) call1(c, o, f_08938A5C, 0x08938968u);
        else call1(c, o, f_08938998, 0x08938978u);
    }
    leave(c, sv);
}

/* (fighter): a move by +192 and +193: +192 = 2 0x08943028, another +192 the crouching one
   (0x08942D54); with +192 clear, standing: with +193 0x08942C80, without it 0x08942894. */
SFA_FUNC(0894281C, move_by_192_4281c) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    int32_t k = S8(o + 192);
    c->r[5] = (uint32_t)k; c->r[6] = 2;
    if (k == 2) call0(c, f_08943028, 0x0894283Cu);
    else if (k != 0) call0(c, f_08942D54, 0x08942858u);
    else {
        int32_t alt = S8(o + 193);
        c->r[5] = (uint32_t)alt;
        *B(o + 696) = 0;
        if (alt != 0) call0(c, f_08942C80, 0x08942874u);
        else call0(c, f_08942894, 0x08942888u);
    }
    leave0(c, ra);
}

/* (fighter): a move by +192, +193 and its step: with +192, crouching, 0x08967024 then 0x0896709C;
   else standing: without +193 0x08966B54; with it 0x08966EF8 for the choice -1 and 0x08966C60 for
   the others. */
SFA_FUNC(08966AB4, move_by_192_66ab4) {
    gaddr o = ARG(0);
    uint32_t ra = enter0(c);
    if (S8(o + 192) != 0) {
        uint8_t step = *B(o + 7);
        *B(o + 696) = 1;
        if (step != 0) call0(c, f_0896709C, 0x08966B48u);
        else call0(c, f_08967024, 0x08966B34u);
    } else {
        int32_t alt = S8(o + 193);
        *B(o + 696) = 0;
        if (alt == 0) call0(c, f_08966B54, 0x08966ADCu);
        else if (S8(o + 386) == -1) call0(c, f_08966EF8, 0x08966B00u);
        else call0(c, f_08966C60, 0x08966B14u);
    }
    leave0(c, ra);
}

/* (fighter): the cpu's pick against its opponent: the character 0xC, unless both are the cpu or
   both players, gets 0x0896B800; the character 0x29 0x0896BA88; any other a random one of the four
   at 0x08A8DD40. */
SFA_FUNC(0896AE7C, pick_by_opponent_6ae7c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    gaddr other = *W(o + 92);
    int32_t ch = S8(other + 338);
    if (ch == 0xC && S8(o + 373) != S8(other + 373)) call1(c, o, f_0896B800, 0x0896AEDCu);
    else if (ch == 0x29) call1(c, o, f_0896BA88, 0x0896AEC4u);
    else {
        call1(c, (uint32_t)ch, f_08821368, 0x0896AEF4u);
        uint32_t fn = *W(0x08A8DD40u + ((c->r[2] & 3) << 2));
        ARG(0) = o; c->r[31] = 0x0896AF14u;
        rt_call_indirect(c, fn);
    }
    leave(c, sv);
}

/* (fighter): the cpu against a player's character 0xD or 7 gets 0x0898958C; everyone else one of
   0x089897BC and 0x08989878 by chance. */
SFA_FUNC(089894F8, pick_by_opponent_894f8) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    gaddr other = *W(o + 92);
    int special = 0;
    if (S8(o + 373) != 0 && S8(other + 373) == 0) {
        int32_t ch = S8(other + 338);
        special = ch == 0xD || ch == 7;
    }
    if (special) call1(c, o, f_0898958C, 0x08989564u);
    else {
        call0(c, f_08821368, 0x08989540u);
        if (c->r[2] & 1) call1(c, o, f_089897BC, 0x08989554u);
        else call1(c, o, f_08989878, 0x0898957Cu);
    }
    leave(c, sv);
}

/* (fighter): on the cue (+77) the next step with +237 = -1: on the ground 0x088D3558 (0x20, 0x30);
   in the air 0x088D36F8, gravity 0.375 and 0x088D3558 (0x1E, 0x40). Then 0x088D3EC0 either way. */
SFA_FUNC(089BBD5C, release_on_cue) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        int32_t air = S8(o + 73);
        *B(o + 237) = 0xFF;
        if (air == 0) call3(c, o, 0x20, 0x30, f_088D3558, 0x089BBDA0u);
        else {
            call1(c, o, f_088D36F8, 0x089BBDB0u);
            *W(o + 108) = 0xFFFFA000u;
            call3(c, o, 0x1E, 0x40, f_088D3558, 0x089BBDC8u);
        }
    }
    call1(c, o, f_088D3EC0, 0x089BBDD0u);
    leave(c, sv);
}
