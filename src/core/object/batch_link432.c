/* The link play's lobby: the screens where the match's rules are set and shown. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME  0x08E75910u
#define PADS  0x08E75BA0u
#define NETP  0x09523980u
#define RULES 0x09523C00u /* the rules the two machines share, nine bytes */
#define SAVED 0x08A058A0u /* the saved options */

void f_088D55F8(Cpu *c); void f_088DD114(Cpu *c); void f_08822EAC(Cpu *c); void f_088DD5D0(Cpu *c);
void f_08823338(Cpu *c); void f_0880A6CC(Cpu *c); void f_088DD124(Cpu *c); void f_089F31AC(Cpu *c);
void f_088E3294(Cpu *c); void f_088E32F0(Cpu *c); void f_088E35B8(Cpu *c); void f_088E336C(Cpu *c);
void f_088E376C(Cpu *c); void f_088E348C(Cpu *c); void f_088E3604(Cpu *c); void f_088E32BC(Cpu *c);
void f_088E5DC8(Cpu *c); void f_088D6950(Cpu *c);

/* (i): the byte at 0x08B03F50 + i. */
SFA_FUNC(088E4988, lobby_table_byte) {
    ARG(1) = 0x08B03F50u;
    ARG(0) += 0x08B03F50u;
    c->r[2] = (uint32_t)S8(ARG(0));
}

/* (value, most, pads): a setting is changed by the pads named in `pads` (a bit each): left
   takes one off and right adds one, each with the cursor's sound; below 0 it wraps to `most`
   and above `most` to 0. The new value in r2. A 0x10 frame keeping s0..s2. */
SFA_FUNC(088DD734, setting_change) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = ARG(0);
    uint32_t pads = ARG(2) & 0xFF;
    *W(sp + 8) = s2;
    c->r[18] = 0;
    *W(sp) = s0; *W(sp + 12) = ra;
    c->r[16] = ARG(1);
    for (uint32_t i = 0; i < 3; i++)
        if ((pads >> i) & 1) c->r[18] |= *W(PADS + 44 * i + 12) | (*W(PADS + 44 * i + 20) & 0xF);
    if ((c->r[18] & 2) != 0) {
        c->r[17]--;
        ARG(0) = 2; c->r[31] = 0x088DD7D8u; f_088D55F8(c);
    }
    if ((c->r[18] & 1) != 0) {
        c->r[17]++;
        ARG(0) = 1; c->r[31] = 0x088DD7ECu; f_088D55F8(c);
    }
    if ((int32_t)c->r[17] < 0) c->r[17] = c->r[16];
    if ((int32_t)c->r[16] < (int32_t)c->r[17]) c->r[17] = 0;
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x10;
}

/* The game's rules are set, from the saved options or to their first values, and copied to the
   nine bytes the two machines share. The link's +45 says whether its +43 is 2 or 4; with it
   set, the game's +118 moves to +48 as 2. Last the link's +265 and +266 become 1 and 0x18
   when its +42 is 2 or 3 and two or more players answered, else 0. */
static void rules_share(Cpu *c, int saved, uint32_t back) {
    uint32_t v = *B(NETP + 43);
    *B(NETP + 45) = v == 2 || v == 4;
    if (saved) {
        *B(GAME + 73) = *B(SAVED + 60); *B(GAME + 45) = *B(SAVED + 60);
        *B(GAME + 47) = *B(SAVED + 61);
        *B(GAME + 77) = *B(SAVED + 62); *B(GAME + 76) = *B(SAVED + 62);
        *B(GAME + 46) = *B(SAVED + 64); *B(GAME + 116) = *B(SAVED + 64);
        *B(GAME + 48) = *B(SAVED + 67);
        *B(GAME + 118) = *B(SAVED + 68);
        *B(GAME + 119) = *B(SAVED + 69);
        *B(GAME + 156) = *B(SAVED + 516);
        if (S8(GAME + 45) < 5) { *B(GAME + 73) = 5; *B(GAME + 45) = 5; }
    } else {
        *B(GAME + 45) = 5; *B(GAME + 73) = 5;
        *B(GAME + 47) = 2;
        *B(GAME + 77) = 1; *B(GAME + 76) = 1;
        *B(GAME + 46) = 1; *B(GAME + 116) = 1;
        *B(GAME + 48) = 0;
        *B(GAME + 118) = 2;
        *B(GAME + 119) = 1;
        *B(GAME + 156) = 0;
    }
    if (S8(GAME + 47) >= 3) *B(GAME + 47) = 2;
    if (*B(NETP + 45) != 0 && S8(GAME + 118) != 0) {
        *B(GAME + 48) = 2;
        *B(GAME + 118) = 0;
    }
    *B(RULES) = *B(NETP + 45);
    *B(RULES + 1) = *B(GAME + 45);
    *B(RULES + 2) = *B(GAME + 47);
    *B(RULES + 3) = *B(GAME + 76);
    *B(RULES + 4) = *B(GAME + 46);
    *B(RULES + 5) = *B(GAME + 48);
    *B(RULES + 6) = *B(GAME + 118);
    *B(RULES + 7) = *B(GAME + 119);
    *B(RULES + 8) = *B(GAME + 156);
    uint32_t kind = *B(NETP + 42);
    int many = 0;
    if (kind == 3 || kind == 2) {
        c->r[31] = back; f_088DD114(c);
        many = (int32_t)c->r[2] >= 2;
    }
    *B(NETP + 265) = (uint8_t)(many ? 1 : 0);
    *B(NETP + 266) = (uint8_t)(many ? 0x18 : 0);
}

/* The menu's cursor (o's +46) is kept off the lines the table at 0x08A3EA34 leaves out for the
   page at o's +47: it moves on the way it was going (`before` is where it was), wrapping at
   0 and 10, until it reaches a line that is there. The cursor's line. */
static int32_t cursor_settle(gaddr o, uint32_t *before) {
    const gaddr lines = 0x08A3EA34u;
    int32_t page = S8(o + 47) * 11;
    int32_t cursor = S8(o + 46);
    if (*B(lines + (uint32_t)(page + cursor)) == 0) {
        int32_t was = cursor;
        do {
            cursor += was < (int32_t)*before ? -1 : 1;
            *B(o + 46) = (uint8_t)cursor;
            cursor = S8(o + 46);
            if (cursor < 0) {
                if (*before == 0xA) { *B(o + 46) = 0; was = -1; }
                else { *B(o + 46) = 0xA; was = 0xB; }
                cursor = S8(o + 46);
            } else if (cursor > 0xA) {
                if (*before == 0) { *B(o + 46) = 0xA; was = 0xB; }
                else { *B(o + 46) = 0; was = -1; }
                cursor = S8(o + 46);
            }
            *before = (uint32_t)was;
            was = cursor;
        } while (*B(lines + (uint32_t)(page + cursor)) == 0);
    }
    return cursor;
}

static void change(Cpu *c, gaddr at, uint32_t most, uint32_t back) {
    ARG(0) = (uint32_t)S8(at); ARG(1) = most; ARG(2) = 1;
    c->r[31] = back; f_088DD734(c);
    *B(at) = (uint8_t)c->r[2];
}

/* (o): the rules screen. The one who made the room (o's +44 at 1) with the link's +43 at 2 or 4
   sets the rules, by the screen's step (o's +0):
     0  the rules are taken from the saved options, the cursor (+46) put at 7; 1 the screen's
        21 objects made and a wait of 0x1E frames set; 2 the wait.
     3  the menu of 11 lines: the cursor moves (skipping the lines the table at 0x08A3EA34
        leaves out for the page at +47), and the line under it changes its rule with left and
        right: 0 to 3 and 5, 6 simple settings; 4 the pair of rules at +5 and +6, which are one
        setting of ten values (or +5 alone when the first rule is set); 7 puts the first values
        back and 8 goes on, both on the button; 9 and 10 the link's +265 and +266. A press of
        back sets step 0x14.
     4  on to 5; 5 the script goes on; 0x14, 0x15 on by one; 0x16 the script goes back, with the
        menu of the screen before made again.
   Anyone else has the rules set from the saved options at once and the script goes on (a step
   of 0x64 sets the script at 0x08A3EB14 instead). Then the screen's title. A 0x40 frame
   keeping s0..s6. */
SFA_FUNC(088D88FC, lobby_rules_screen) {
    gaddr o = ARG(0);
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    *W(sp + 36) = sv[1]; *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4];
    c->r[17] = 1; c->r[19] = 2; c->r[20] = 4;
    *W(sp + 40) = sv[2]; *W(sp + 52) = sv[5]; *W(sp + 56) = sv[6]; *W(sp + 60) = ra;
    uint32_t step = *H(o);
    int sets = 0;
    if (*B(o + 44) == 1) {
        c->r[18] = NETP;
        sets = *B(NETP + 43) == 2 || *B(NETP + 43) == 4;
    }
    if (!sets) {
        c->r[21] = 0x08A3EA04u;
        gaddr title;
        if (step == 0x64) {
            *W(o + 20) = 0x08A3EB14u;
            *H(o + 24) = 0;
            *H(o + 2) = 0;
            *H(o) = 0;
            *B(o + 47) = 0;
            title = *W(0x08A3EA04u + (((uint32_t)*B(o + 42) + *B(o + 43)) << 2));
        } else {
            c->r[18] = NETP;
            rules_share(c, 1, 0x088D9304u);
            c->r[17] = *H(o + 24) + 1u;
            gaddr at = 0x08A3EA04u + (((uint32_t)*B(o + 42) + *B(o + 43)) << 2);
            *H(o + 24) = (uint16_t)c->r[17];
            *H(o + 2) = 0;
            *H(o) = 0;
            *B(o + 47) = 0;
            title = *W(at);
        }
        c->r[16] = title;
        ARG(0) = title; ARG(1) = 2; ARG(2) = 0x30; ARG(3) = 4; c->r[8] = 9;
        c->r[31] = 0x088D9388u; f_0880A6CC(c);
        goto done;
    }
    switch (step) {
    case 0: {
        c->r[21] = 0x1E;
        rules_share(c, 1, 0x088D8AACu);
        uint32_t next = *H(o) + 1u;
        c->r[18] = (uint32_t)*B(o + 42) + *B(o + 43) + 1;
        *B(o + 46) = 7;
        *B(o + 47) = (uint8_t)c->r[18];
        *H(o) = (uint16_t)next;
    }
        /* fall through */
    case 1: {
        c->r[21] = 0x1E;
        *B(o + 263) = 1;
        static const uint8_t parts[21] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                                           0x1E, 0x1F, 0x21, 0x22, 0x23, 0x24 };
        for (uint32_t i = 0; i < 21; i++) {
            ARG(0) = 0x9C; ARG(1) = parts[i];
            c->r[31] = 0x088D8B24u + 12 * i; f_08822EAC(c);
        }
        *H(o + 4) = 0x1E;
        *H(o) = (uint16_t)(*H(o) + 1);
    }
        /* fall through */
    case 2:
        c->r[17] = (uint32_t)S16(o + 4) - 1;
        *H(o + 4) = (uint16_t)c->r[17];
        if ((int16_t)c->r[17] == 0) *H(o) = (uint16_t)(*H(o) + 1);
        break;
    case 3: {
        c->r[21] = RULES;
        c->r[22] = (uint32_t)S8(o + 46);
        ARG(0) = o; ARG(1) = 0xB; ARG(2) = 1;
        c->r[31] = 0x088D8C74u; f_088DD5D0(c);
        int32_t pressed = (int32_t)c->r[2];
        int32_t both = S8(RULES + 6) + (S8(RULES + 5) != 0 ? S8(RULES + 5) + 2 : 0);
        if (pressed < 0) { *H(o) = 0x14; break; }
        int32_t cursor = cursor_settle(o, &c->r[22]);
        switch (cursor) {
        case 0:
            change(c, RULES, 1, 0x088D8DA4u);
            if (S8(RULES) != 0 && S8(RULES + 6) != 0) { *B(RULES + 5) = 2; *B(RULES + 6) = 0; }
            break;
        case 1: change(c, RULES + 2, 2, 0x088D8DDCu); break;
        case 2: change(c, RULES + 3, 2, 0x088D8DF4u); break;
        case 3: change(c, RULES + 4, 3, 0x088D8E0Cu); break;
        case 4:
            if (S8(RULES) != 0) {
                change(c, RULES + 5, 7, 0x088D8E30u);
                *B(RULES + 6) = 0;
            } else {
                ARG(0) = (uint32_t)both; ARG(1) = 9; ARG(2) = 1;
                c->r[31] = 0x088D8E48u; f_088DD734(c);
                int32_t got = (int32_t)c->r[2];
                if (got == 0) { *B(RULES + 6) = 0; *B(RULES + 5) = 0; }
                else if (got < 3) { *B(RULES + 6) = (uint8_t)got; *B(RULES + 5) = 0; }
                else { *B(RULES + 6) = 0; *B(RULES + 5) = (uint8_t)(got - 2); }
                if (S8(RULES + 6) != 0) *B(RULES + 5) = 0;
                if (S8(RULES + 5) != 0) *B(RULES + 6) = 0;
            }
            break;
        case 5: change(c, RULES + 7, 1, 0x088D8EB4u); break;
        case 6: change(c, RULES + 8, 1, 0x088D8ECCu); break;
        case 7:
            if (pressed > 0) rules_share(c, 0, 0x088D8FD8u);
            break;
        case 8:
            if (pressed > 0) {
                *H(o + 50) = (uint16_t)(S16(o + 50) + *B(o + 55));
                *B(o + 46) = 0;
                *H(o) = (uint16_t)(*H(o) + 1);
            }
            break;
        case 9: change(c, o + 265, 5, 0x088D9038u); break;
        case 10: change(c, o + 266, 0x3B, 0x088D9050u); break;
        default: break;
        }
        break;
    }
    case 4:
    case 20:
        *B(o + 47) = 0;
        *H(o) = (uint16_t)(step + 1);
        break;
    case 5:
        *H(o + 2) = 0;
        *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
        *H(o) = 0;
        *B(o + 47) = 0;
        break;
    case 21:
        *H(o) = (uint16_t)(step + 1);
        break;
    case 22:
        *H(o + 24) = (uint16_t)(*H(o + 24) - 1);
        *H(o + 2) = 0;
        *H(o) = 4;
        *B(o + 46) = 0;
        *B(o + 45) = 0;
        *B(o + 47) = 1;
        for (uint32_t i = 0; i < 6; i++) {
            ARG(0) = 0x20; ARG(1) = i;
            c->r[31] = 0x088D90D4u + 12 * i; f_08823338(c);
        }
        break;
    default:
        *W(o + 20) = 0x08A3EB14u;
        *H(o + 24) = 0;
        *H(o + 2) = 0;
        *H(o) = 0;
        *B(o + 47) = 0;
        break;
    }
    if (*B(o + 43) == 4) {
        ARG(0) = 0x08A01060u; ARG(1) = 2; ARG(2) = 0x38; ARG(3) = 4; c->r[8] = 9;
        c->r[31] = 0x088D915Cu; f_0880A6CC(c);
    } else {
        ARG(0) = 0x08A01074u; ARG(1) = 2; ARG(2) = 0x58; ARG(3) = 4; c->r[8] = 9;
        c->r[31] = 0x088D917Cu; f_0880A6CC(c);
    }
done:
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}

/* The rules the two machines share are taken into the game, and the room's size noted: o's
   +265 and +266 become 1 and 0x18 when o's +42 is 2 or 3 and two or more players answered. */
static void rules_take(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 46) = 7;
    *B(o + 47) = (uint8_t)((uint32_t)*B(o + 42) + *B(o + 43) + 1);
    *B(o + 45) = *B(RULES);
    *B(GAME + 73) = *B(RULES + 1); *B(GAME + 45) = *B(RULES + 1);
    *B(GAME + 47) = *B(RULES + 2);
    *B(GAME + 77) = *B(RULES + 3); *B(GAME + 76) = *B(RULES + 3);
    *B(GAME + 46) = *B(RULES + 4); *B(GAME + 116) = *B(RULES + 4);
    *B(GAME + 48) = *B(RULES + 5);
    *B(GAME + 118) = *B(RULES + 6);
    *B(GAME + 119) = *B(RULES + 7);
    *B(GAME + 156) = *B(RULES + 8);
    uint32_t kind = *B(o + 42);
    int many = 0;
    if (kind == 3 || kind == 2) {
        c->r[31] = back; f_088DD114(c);
        many = (int32_t)c->r[2] >= 2;
    }
    *B(o + 265) = (uint8_t)(many ? 1 : 0);
    *B(o + 266) = (uint8_t)(many ? 0x18 : 0);
    *H(o) = (uint16_t)(*H(o) + 1);
}

/* A wait at o's +4 goes down by one; whether it has run out. */
static int wait_over(gaddr o) {
    int32_t left = S16(o + 4) - 1;
    *H(o + 4) = (uint16_t)left;
    return (int16_t)left == 0;
}

static void script_on(gaddr o) {
    *H(o + 2) = 0;
    *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
    *H(o) = 0;
    *B(o + 47) = 0;
}

/* (o): the room's screen before a match, where the rules are shown and, for the one who made
   the room, can still be changed. Its line of text (how many players are in the room) is made
   on the first step, when the shared rules are taken into the game.
   With the link's +43 at 2 or 4 the rules are only shown: steps 0 to 2 set the screen up with
   19 objects and a wait of 0x168 frames, 3 is the wait, 4 the script goes on.
   Otherwise: 0 to 2 set it up with 21 objects and a wait of 0x1E frames; 3 that wait, then one
   of 0x708 frames; 4 the menu of 11 lines until the wait runs out (then a wait of 0x3C): the
   lines change the game's own rules as on the rules screen, line 0 is skipped, 7 puts the
   first values back and 8 ends the wait at once, both on the button; 5 the last wait; 6 and 7
   the two machines made ready; 8 the script goes on. A step past the last ends the link's
   part and sets the script at 0x08A3EB14.
   Then the line of text, and with the page (+47) set the screen's own drawing, in one of two
   forms by the link's +43 and at a height by the room's size. A 0x50 frame keeping s0..s7 and
   fp, with the link's record and the text at its +32 and +36. */
SFA_FUNC(088DB05C, lobby_room_screen) {
    gaddr o = ARG(0);
    uint32_t sv[8], fp = c->r[30], ra = c->r[31];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 44) = sv[1];
    c->r[17] = o;
    *W(sp + 48) = sv[2];
    c->r[18] = GAME;
    *W(sp + 72) = fp;
    *W(sp + 40) = sv[0]; *W(sp + 52) = sv[3]; *W(sp + 56) = sv[4]; *W(sp + 60) = sv[5];
    *W(sp + 64) = sv[6]; *W(sp + 68) = sv[7]; *W(sp + 76) = ra;
    c->r[30] = (uint32_t)(int16_t)(S16(0x09523438u + 22) + S16(0x09523438u + 230));
    ARG(0) = (uint32_t)(S16(0x09523438u + 22) + S16(0x09523438u + 230));
    c->r[31] = 0x088DB0B4u; f_088DD124(c);
    uint32_t players = c->r[2];
    uint32_t mode = *B(NETP + 43);
    *W(sp + 32) = NETP;
    uint32_t step = *H(o);
    const gaddr text = 0x08A3EB44u;
    c->r[23] = 2;
    c->r[21] = 5;
    *W(sp + 36) = text;
    static const uint8_t parts[21] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                                       0x1E, 0x1F, 0x21, 0x22, 0x23, 0x24 };
    int shown = mode == 2;
    if (!shown) { c->r[22] = 4; shown = mode == 4; }
    if (shown) {
        if (step >= 5) {
            c->r[31] = 0x088DB3A0u; f_088E35B8(c);
            *W(o + 20) = 0x08A3EB14u;
            *H(o + 24) = 0; *H(o + 2) = 0; *H(o) = 0;
            *B(o + 47) = 0;
        } else if (step == 3) {
            if (wait_over(o)) { *B(o + 47) = 0; *H(o) = (uint16_t)(step + 1); }
        } else if (step == 4) {
            script_on(o);
        } else {
            c->r[21] = 0x168;
            if (step == 0) {
                c->r[20] = 0x08A0109Cu; c->r[19] = RULES; c->r[16] = 3;
                ARG(0) = players;
                c->r[31] = 0x088DB150u; f_088E4988(c);
                ARG(0) = text; ARG(1) = 0x08A0109Cu; ARG(2) = c->r[2] + 1;
                c->r[31] = 0x088DB168u; f_089F31AC(c);
                c->r[18] = *B(o + 42);
                rules_take(c, o, 0x088DB1F4u);
            }
            if (step <= 1) {
                c->r[31] = 0x088DB23Cu; f_088E3294(c);
                ARG(0) = c->r[30]; c->r[31] = 0x088DB244u; f_088E32F0(c);
                *H(o) = (uint16_t)(*H(o) + 1);
            }
            *B(o + 263) = 0;
            for (uint32_t i = 0, n = 0; i < 21; i++) {
                if (parts[i] == 4 || parts[i] == 5) continue;
                ARG(0) = 0x9C; ARG(1) = parts[i];
                c->r[31] = 0x088DB268u + 12 * n++; f_08822EAC(c);
            }
            *H(o + 4) = 0x168;
            *H(o) = (uint16_t)(*H(o) + 1);
        }
    } else {
        int32_t both = S8(GAME + 118) + (S8(GAME + 48) != 0 ? S8(GAME + 48) + 2 : 0);
        c->r[19] = (uint32_t)both;
        switch (step) {
        case 0:
            c->r[20] = 0x08A0109Cu; c->r[19] = RULES; c->r[16] = 3; c->r[21] = 0x1E;
            ARG(0) = text; ARG(1) = 0x08A0109Cu; ARG(2) = players + 1;
            c->r[31] = 0x088DB43Cu; f_089F31AC(c);
            c->r[20] = *B(o + 42);
            rules_take(c, o, 0x088DB4C8u);
            /* fall through */
        case 1:
            c->r[21] = 0x1E;
            c->r[31] = 0x088DB510u; f_088E3294(c);
            ARG(0) = c->r[30]; c->r[31] = 0x088DB518u; f_088E32F0(c);
            if (S8(o + 36) == 0) { ARG(0) = GAME; ARG(1) = 1; c->r[31] = 0x088DB52Cu; f_088E336C(c); }
            *H(o) = (uint16_t)(*H(o) + 1);
            /* fall through */
        case 2:
            c->r[21] = 0x1E;
            *B(o + 263) = 0;
            for (uint32_t i = 0; i < 21; i++) {
                ARG(0) = 0x9C; ARG(1) = parts[i];
                c->r[31] = 0x088DB550u + 12 * i; f_08822EAC(c);
            }
            *H(o + 4) = 0x1E;
            *H(o) = (uint16_t)(*H(o) + 1);
            break;
        case 3:
            if (wait_over(o)) { *H(o + 4) = 0x708; *H(o) = (uint16_t)(step + 1); }
            break;
        case 4: {
            if (wait_over(o)) { *H(o + 4) = 0x3C; *H(o) = (uint16_t)(step + 1); break; }
            c->r[16] = (uint32_t)S8(o + 46);
            ARG(0) = o; ARG(1) = 0xB; ARG(2) = 1;
            c->r[31] = 0x088DB6C0u; f_088DD5D0(c);
            c->r[20] = c->r[2];
            c->r[31] = 0x088DB6C8u; f_088E376C(c);
            if (c->r[2] == 0) c->r[20] = 0;
            int32_t pressed = (int32_t)c->r[20];
            int32_t cursor = cursor_settle(o, &c->r[16]);
            switch (cursor) {
            case 0:
                *B(o + 46) = (uint8_t)(c->r[16] == 0xA ? 1 : 8);
                break;
            case 1: change(c, GAME + 47, 2, 0x088DB7F4u); break;
            case 2:
                change(c, GAME + 76, 2, 0x088DB80Cu);
                *B(GAME + 77) = *B(GAME + 76);
                break;
            case 3:
                change(c, GAME + 46, 3, 0x088DB828u);
                *B(GAME + 116) = *B(GAME + 46);
                break;
            case 4:
                if (*B(o + 45) != 0) {
                    if (S8(GAME + 118) != 0) { *B(GAME + 48) = 2; *B(GAME + 118) = 0; }
                    change(c, GAME + 48, 7, 0x088DB864u);
                    *B(GAME + 118) = 0;
                } else {
                    ARG(0) = (uint32_t)both; ARG(1) = 9; ARG(2) = 1;
                    c->r[31] = 0x088DB87Cu; f_088DD734(c);
                    int32_t got = (int32_t)c->r[2];
                    c->r[19] = c->r[2];
                    if (got == 0) { *B(GAME + 118) = 0; *B(GAME + 48) = 0; }
                    else if (got < 3) { *B(GAME + 118) = (uint8_t)got; *B(GAME + 48) = 0; }
                    else { *B(GAME + 118) = 0; *B(GAME + 48) = (uint8_t)(got - 2); }
                    if (S8(GAME + 118) != 0) *B(GAME + 48) = 0;
                    if (S8(GAME + 48) != 0) *B(GAME + 118) = 0;
                }
                break;
            case 5: change(c, GAME + 119, 1, 0x088DB8E8u); break;
            case 6: change(c, GAME + 156, 1, 0x088DB900u); break;
            case 7:
                if (pressed > 0) {
                    c->r[16] = NETP;
                    c->r[18] = 1;
                    rules_share(c, 0, 0x088DBA14u);
                }
                break;
            case 8:
                if (pressed > 0) { *H(o + 4) = 1; *H(o) = (uint16_t)(*H(o) + 1); }
                break;
            case 9: {
                change(c, o + 265, 5, 0x088DBA68u);
                uint32_t kind = *B(o + 42);
                if (kind == 3 || kind == 2) {
                    c->r[31] = 0x088DBA88u; f_088DD114(c);
                    if ((int32_t)c->r[2] >= 2 && S8(o + 265) < 1) *B(o + 265) = 1;
                }
                break;
            }
            case 10: change(c, o + 266, 0x3B, 0x088DBABCu); break;
            default: break;
            }
            break;
        }
        case 5:
            if (wait_over(o)) { *B(o + 47) = 0; *H(o) = (uint16_t)(step + 1); }
            break;
        case 6:
            ARG(0) = GAME; ARG(1) = 0; c->r[31] = 0x088DBAF8u; f_088E348C(c);
            *H(o) = (uint16_t)(*H(o) + 1);
            /* fall through */
        case 7:
            c->r[31] = 0x088DBB0Cu; f_088E3604(c);
            if (c->r[2] != 0) {
                c->r[31] = 0x088DBB1Cu; f_088E32BC(c);
                *H(o) = (uint16_t)(*H(o) + 1);
            }
            break;
        case 8:
            script_on(o);
            break;
        default:
            c->r[31] = 0x088DBB50u; f_088E35B8(c);
            *W(o + 20) = 0x08A3EB14u;
            *H(o + 24) = 0; *H(o + 2) = 0; *H(o) = 0;
            *B(o + 47) = 0;
            break;
        }
    }
    ARG(0) = *W(sp + 36); ARG(1) = 2; ARG(2) = 0x78; ARG(3) = 5; c->r[8] = 0xB;
    c->r[31] = 0x088DBB84u; f_0880A6CC(c);
    if (S8(o + 47) != 0) {
        gaddr link = *W(sp + 32);
        uint32_t m = *B(link + 43);
        uint32_t kind = *B(o + 42);
        if (S8(o + 36) == 0 && *B(o + 32) != 0 && m != 2 && m != 4) {
            *B(o + 58) = 1;
            m = *B(link + 43);
        }
        if (m == 2 || m == 4) {
            c->r[16] = 0x4E;
            if (kind == 3 || kind == 2) {
                c->r[31] = 0x088DBBF4u; f_088DD114(c);
                if ((int32_t)c->r[2] >= 2) c->r[16] = 0x30;
            }
            ARG(0) = o; ARG(1) = c->r[16];
            c->r[31] = 0x088DBC10u; f_088E5DC8(c);
        } else {
            c->r[16] = 0x60;
            if (kind == 3 || kind == 2) {
                c->r[31] = 0x088DBC30u; f_088DD114(c);
                if ((int32_t)c->r[2] >= 2) c->r[16] = 0x42;
            }
            ARG(0) = o; ARG(1) = c->r[16];
            c->r[31] = 0x088DBC4Cu; f_088D6950(c);
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = fp; c->r[31] = ra; c->r[29] += 0x50;
}
