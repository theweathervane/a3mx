/* A player's cursor on the character select screen: the steps of its states (screen.c runs the
 * states and their dispatchers). Each player goes through four choices in turn, each a state of
 * three or four steps (fighter + 4 the state, + 5 the step):
 *
 *   state 0  the character  enter 0x08894644, move 0x08894768 (grid 0x08897008, chosen 0x08897344),
 *                           hand over 0x0889494C (screen.c), wait 0x08894990
 *   state 1  the play style enter 0x08894FC0, choose 0x0889507C (chosen 0x088951DC), wait 0x08895248
 *   state 2  the ISM        enter 0x08894C74, choose 0x08894D40 (chosen 0x08894E5C), wait 0x08894F4C
 *   state 3  the speed      enter 0x08895338, choose 0x088953CC, wait 0x088954A0
 *   state 4  done           0x088954E0 (screen.c), which counts the choice (0x088969FC)
 *
 * An "enter" step skips its choice when it is already made (fighter + 424) or, with BATTLE + 90, takes
 * it from the other player; a "wait" step holds the choice on screen for a moment (+82) and moves on.
 * In a network battle each choice is also made for the player when its time runs out.
 *
 * A random box draws a character each frame (0x08894A84); an ISM decided rather than chosen (an
 * entry's, a random box's, CLASSICAL's) is applied by 0x08895564.
 */
#include "select.h"

#define GRID           0x08A26F60u   /* s8 [7 rows][7 columns]: each box's character, or -1 for none;
                                        bit 0x40: a random box (kind in the low nibble) for side bit 0;
                                        negative (not -1), network only: character (>> 1) & 0x3F for side bit 0 */
#define ENTRY_BOXES    0x08A26FC2u   /* s8 [6]: the grid boxes (row × 7 + column) of the six entries */
#define BOX_OF         0x08A270B7u   /* u8 [2] by character (or random kind + 0x2A): its column, row */
#define RANDOM_POOL    0x08A271FCu   /* s8 [38]: a random box's characters; negative: hidden, & 0x7F indexes
                                        HIDDEN (if SYS + 73 + it is set) */
#define HIDDEN         0x08A27222u   /* u8: the hidden characters */
#define RANDOM_ISM     0x08A27228u   /* s8 [32]: by (random kind & 2) × 8 + GAME + 32's low nibble */
#define SPEEDS         0x08A2727Cu   /* u8, by GAME + 20's choice + GAME + 118 - 1: BATTLE + 22 */
#define ENTRIES        0x09358528u   /* 0x354 bytes per slot: the entry a player plays as (+0 in use, +2 its ISM, +3 its colour) */
#define SAVED_ENTRIES  0x09519588u   /* six entries of 0x354 bytes (EDIT / ENTRY) */
#define NET_ENTRIES    0x09523454u   /* per side 208 bytes: three entries of 0x3C */
#define ENTRY_SIZE     0x354u
#define NET_ENTRY_SIZE 0x3Cu

#define PADS           0x08E75BA0u   /* 44 bytes each: +4 held, +12 newly pressed */
#define PAD_SIZE       0x2Cu
#define PAD_HELD       4
#define PAD_PRESSED    12

#define BUTTONS_CHOOSE 0x3F0u        /* the six attack buttons */
#define BUTTON_SELECT  0x4000u
#define DIR_RIGHT 1
#define DIR_LEFT  2
#define DIR_DOWN  4
#define DIR_UP    8

#define WAIT_CHOICE    0x258         /* frames to choose (network) */
#define WAIT_SHOWN     0x20          /* frames a choice stays on screen */
#define WAIT_CHARACTER 0x4B0         /* frames to choose a character (network) */

#define SOUND_ISM      0x125

enum {   /* characters the screen changes into others */
    CH_AKUMA = 2, CH_CHUNLI = 4, CH_SODOM = 6, CH_BISON = 0xA, CH_GEN = 0x11,
    CH_CHUNLI_X = 0x12, CH_GEN_X = 0x13, CH_SODOM_X = 0x14, CH_BALROG = 0x15,
    CH_SHIN_AKUMA = 0x24, CH_BALROG_2 = 0x25, CH_FINAL_BISON = 0x26,
};

void f_088D5E44(Cpu *c);   /* a network battle (fight.c) */
void f_088D7C1C(Cpu *c);   /* a continuing match (fight.c) */
void f_08896AA4(Cpu *c);   /* below: (fighter) its pad's newly pressed buttons */
void f_08896B18(Cpu *c);   /* below: (fighter) its pad's held buttons */
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
void f_089E0DC4(Cpu *c);   /* memcpy */
void f_08821368(Cpu *c);   /* a random number (fight.c) */
void f_0889472C(Cpu *c); void f_08897008(Cpu *c); void f_08897344(Cpu *c); void f_08894768(Cpu *c);
void f_08894E5C(Cpu *c); void f_088951DC(Cpu *c); void f_088A4F1C(Cpu *c);   /* below */
void f_08894A84(Cpu *c); void f_08895564(Cpu *c);                             /* below */
void f_08894F8C(Cpu *c); void f_08894C40(Cpu *c); void f_08895304(Cpu *c);   /* states 1-3 (screen.c) */
void f_088954E0(Cpu *c);   /* state 4 (screen.c) */

static void set_flag8(gaddr a, uint8_t bits)   { *B(a) = (uint8_t)(S8(a) | bits); }
static void set_flag16(gaddr a, uint16_t bits) { *H(a) = (uint16_t)(S16(a) | bits); }

/* A step's timer: one frame less, and whether it has run out. */
static int timer_out(gaddr me) {
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    return t == 0;
}

/* The next state, from its first step. */
static void next_state(gaddr me) {
    *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
    *B(me + F_STEP) = 0;
}

static void next_step(gaddr me) { *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1); }

static int16_t wrap_next(int16_t v) { v = (int16_t)(v + 1); return v < 7 ? v : 0; }
static int16_t wrap_prev(int16_t v) { v = (int16_t)(v - 1); return v >= 0 ? v : 6; }

/* Two players on the same character (Balrog's two numbers count as one) with the same colours: the
   one choosing now takes the colours of the button next to his. Only once the other has chosen his
   character or ISM. (0x08895564 leaves + 449 as it was.) */
static void avoid_same_colours(gaddr me, gaddr other, int copy) {
    if ((S8(other + F_CHOSEN) & (CHOSE_CHARACTER | CHOSE_ISM)) == 0) return;
    if (S8(other + F_COLOUR) != S8(me + F_COLOUR)) return;
    int8_t a = S8(me + F_CHARACTER), b = S8(other + F_CHARACTER);
    if (a == b || (a == CH_BALROG && b == CH_BALROG_2) || (a == CH_BALROG_2 && b == CH_BALROG)) {
        *B(me + F_COLOUR) = (uint8_t)(S8(me + F_COLOUR) ^ 1);
        if (copy) *B(me + F_COLOUR_COPY) = *B(me + F_COLOUR);
    }
}


/* The ISM is decided, not chosen: style 3, an entry, or a random box (not marked + 80 / + 126). */
static int ism_decided(gaddr me) {
    if (S8(me + F_ISM_FIXED) != 0 || S8(me + F_ENTRY_BOX) != 0) return 1;
    return S8(me + F_RANDOM_BOX) != 0 && S8(me + F_RANDOM_FLAG) == 0 && S8(me + F_RANDOM_HIGH) == 0;
}

static void decide_ism(Cpu *c, gaddr me, gaddr other) {
    set_flag8(me + F_CHOSEN, CHOSE_ISM);
    *B(me + F_LAST_ISM) = *B(me + F_ISM);
    CALL2(f_08895564, me, other);
}

/* ---- the pads ---- */

/* A box's pad (0x08896AA4 pressed / 0x08896B18 held, by `word`): in the team modes (Dramatic, Variable)
   both boxes read the starter's (player 1's if present), otherwise the slot's (0, or 1 for any other).
   selectscreen.c wraps both: offline in Dramatic Battle, the pad playing the box. */
static uint32_t box_pad(gaddr me, uint32_t word) {
    int8_t mode = S8(GAME + G_MODE);
    int first;
    if (mode == MODE_DRAMATIC || mode == MODE_VARIABLE) first = (S8(GAME + G_PRESENT) & 1) != 0;
    else first = S8(me + F_SLOT) == 0;
    return *W(PADS + (first ? 0 : PAD_SIZE) + word) & 0xFFFF;
}

/* ---- state 0: the character ---- */

/* Whether a character can't be picked (0x0889472C): its bit in GAME + 148 / + 152. */
static uint32_t character_barred(uint32_t ch) {
    if ((int32_t)ch < 32) return *W(GAME + G_BARRED) & (1u << (ch & 31));
    return *W(GAME + G_BARRED + 4) & (1u << ((ch - 32) & 31));
}

/* Enter (0x08894644): the cursor on the character's box (or the random box it came back from); on to
   the style if the character is already chosen, otherwise the move step at once. */
static void character_enter(Cpu *c, gaddr me, gaddr other) {
    *B(me + F_ENTRY_BOX) = 0;
    *B(me + F_RANDOM_HIGH) = 0;
    *B(me + F_RANDOM_FLAG) = 0;
    int32_t at = S8(me + F_CHARACTER);
    *B(me + 73) = 0;
    *B(me + F_RANDOM_BOX) = 0;
    int8_t box = S8(me + F_BOX);
    if (box != 0) {
        *B(me + F_RANDOM_HIGH) = (uint8_t)(box & 0x80);
        *B(me + F_RANDOM_BOX) = (uint8_t)(box & 0x3F);
        *B(me + F_BOX) = (uint8_t)(box & 0x3F);
        at = box & 0x3F;
    }
    gaddr place = BOX_OF + (uint32_t)at * 2u;
    *H(me + F_COLUMN) = *B(place);
    *H(me + F_ROW) = *B(place + 1);
    if (S16(me + F_DONE) & DONE_CHARACTER) {
        *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
        CALL2(f_08894F8C, me, other);
        return;
    }
    next_step(me);
    *H(me + F_TIMER) = WAIT_CHARACTER;
    *B(me + F_BOX) = *B(me + F_RANDOM_BOX);
    CALL2(f_08894768, me, other);
}

/* Choose the character under the cursor, unless a network session bars it. */
static void choose_if_allowed(Cpu *c, gaddr me, gaddr other, uint32_t buttons) {
    if (S8(GAME + G_NET) != 0 && CALL1(f_0889472C, (uint32_t)(int32_t)S8(me + F_CHARACTER)) != 0) return;
    CALL3(f_08897344, me, other, buttons);
}

/* Move (0x08894768): which entry's box the cursor is on; then an attack button chooses, a direction
   moves, and on a random box a character is drawn each frame. A network battle chooses when time
   runs out. (The original looks the entry boxes up in one of two loops by 0x088D7C1C; both read the
   same table.) */
static void character_move(Cpu *c, gaddr me, gaddr other) {
    if (CALL0(f_088D5E44) != 0 && timer_out(me)) {
        choose_if_allowed(c, me, other, 0x20);
        return;
    }
    uint32_t at = (uint16_t)(S16(me + F_ROW) * 7 + S16(me + F_COLUMN));
    uint32_t k = 0;
    while (k < 6 && (uint32_t)(int32_t)S8(ENTRY_BOXES + k) != at) k++;
    *B(me + F_ENTRY_BOX) = (uint8_t)(k == 6 ? 0 : k + 1);

    uint32_t pressed = CALL1(f_08896AA4, me);
    if (pressed & BUTTONS_CHOOSE) {
        choose_if_allowed(c, me, other, pressed & BUTTONS_CHOOSE);
    } else if (pressed & 0xF) {
        CALL3(f_08897008, me, other, pressed & 0xF);
    } else if (S8(me + F_RANDOM_BOX) != 0) {
        *B(me + F_CHARACTER) = (uint8_t)CALL2(f_08894A84, me, other);
    }
}

/* A random box's character (0x08894A84), drawn anew each frame unless one was kept (+ 429): from
   the pool, hidden characters only once unlocked; and its ISM: CLASSICAL's, the network's, the one
   chosen (if the box is marked), or drawn by the kind and the frame; with BATTLE + 90 the other
   player's once he has one. The X-ISM character for it. */
static int32_t random_character(Cpu *c, gaddr me, gaddr other) {
    int32_t ch = S8(me + F_CHARACTER);
    if (S8(me + F_BOX) == 0) {
        for (;;) {
            uint32_t r = CALL0(f_08821368) & 0x3F;
            if (r >= 38) continue;
            ch = S8(RANDOM_POOL + r);
            if (ch >= 0) break;
            uint32_t hidden = (uint32_t)ch & 0x7F;
            if (S8(SYS + 73 + hidden) == 0) continue;
            ch = S8(HIDDEN + hidden);
            break;
        }
    }
    int32_t ism = S8(me + F_ISM);
    if (S8(me + F_ISM_FIXED) != 0) {
        ism = -1;
    } else if (S8(GAME + G_NET) != 0 && S8(GAME + G_NET_LOBBY) >= 2) {
        uint8_t v = (uint8_t)(S8(GAME + G_NET_LOBBY) - 3);
        *B(me + F_ISM) = v;
        *B(me + F_LAST_ISM) = v;
        ism = (int8_t)v;
    } else if (S8(me + F_RANDOM_FLAG) == 0 && S8(me + F_RANDOM_HIGH) == 0 && S8(me + F_BOX) == 0) {
        int32_t at = (int8_t)(((S8(me + F_RANDOM_BOX) & 2) << 3) + (*B(GAME + 32) & 0xF));
        ism = S8(RANDOM_ISM + (uint32_t)at);
    }
    if (S8(BATTLE + BT_P2_FIRST) != 0 && (S8(other + F_CHOSEN) & CHOSE_ISM)) ism = S8(other + F_ISM);
    *B(me + F_ISM) = (uint8_t)ism;
    int32_t to = x_ism_character(ch, ism);
    return to >= 0 ? to : ch;
}

/* The grid's box for this player (-1: none): random boxes only on their own side; in a network
   session, the boxes only it has. */
static int32_t box_for(int32_t e, int8_t slot, int net) {
    if ((e & 0x40) && (e & 1) != slot) e = -1;
    if (net && e < 0 && e != -1) e = (e & 1) == slot ? (e >> 1) & 0x3F : -1;
    return e;
}

/* Grid (0x08897008, a2 the directions): the cursor moves until it lands on a box (wrapping round;
   having gone right round a row or column, it tries the next one), and the box's character (a
   random box: + 81 its kind, and a character drawn) becomes the player's, with the ISM offered:
   the last one if the character is the same, the network's otherwise. */
static void character_grid(Cpu *c, gaddr me, gaddr other, uint32_t dirs) {
    dirs &= 0xFFFF;
    int right = dirs & DIR_RIGHT, left = dirs & DIR_LEFT, down = dirs & DIR_DOWN, up = dirs & DIR_UP;
    int across = dirs & (DIR_RIGHT | DIR_LEFT), along = dirs & (DIR_DOWN | DIR_UP);
    int16_t col = S16(me + F_COLUMN), row = S16(me + F_ROW);
    int32_t e;
    for (;;) {
        if (right) col = wrap_next(col);
        if (left)  col = wrap_prev(col);
        if (down)  row = wrap_next(row);
        if (up)    row = wrap_prev(row);
        e = box_for(S8(GRID + (uint32_t)(row * 7 + col)), S8(me + F_SLOT), CALL0(f_088D7C1C) != 0);
        if (e >= 0) break;
        if (col == S16(me + F_COLUMN) && across) {
            if (!along) row = wrap_next(row);
            else { if (down) row = wrap_next(row); if (up) row = wrap_prev(row); }
        }
        if (row == S16(me + F_ROW) && along) {
            if (!across) col = wrap_next(col);
            else { if (right) col = wrap_next(col); if (left) col = wrap_prev(col); }
        }
    }
    *B(me + F_BOX) = 0;
    *B(me + F_RANDOM_HIGH) = 0;
    *B(me + F_RANDOM_BOX) = 0;
    if (e & 0x40) {
        *B(me + F_RANDOM_BOX) = (uint8_t)((e & 0xF) + 0x2A);
        e = (int32_t)CALL2(f_08894A84, me, other);
    }
    *B(me + F_CHARACTER) = (uint8_t)e;
    *H(me + F_COLUMN) = (uint16_t)col;
    *H(me + F_ROW) = (uint16_t)row;

    int32_t ism = 0;
    if ((uint32_t)(int32_t)S8(me + F_LAST_CHARACTER) == (uint32_t)e && S8(me + F_RANDOM_BOX) == 0)
        ism = S8(me + F_LAST_ISM);
    if (S8(GAME + G_NET) != 0) {
        int8_t lobby = S8(GAME + G_NET_LOBBY);
        if (lobby >= 2) ism = (int16_t)(lobby - 3);
        if (S8(GAME + G_NET_RULE) == 3) { *B(me + F_ISM_FIXED) = 1; ism = -1; }
    }
    *B(me + F_ISM) = (uint8_t)ism;
}

/* The colours, by the button that chose (later buttons win). */
static uint8_t colour_of(uint32_t buttons) {
    uint8_t colour = 0;
    if (buttons & 0x20)  colour = 2;
    if (buttons & 0x40)  colour = 4;
    if (buttons & 0x80)  colour = 1;
    if (buttons & 0x100) colour = 3;
    if (buttons & 0x200) colour = 5;
    return colour;
}

/* A character with SELECT held (where the unlock allows): Akuma becomes Shin Akuma and the second
   Balrog the first, unless a network session bars them; M. Bison is Final Bison in Arcade, VS and
   modes 9, 0x11, 0x12 (in a network session, if allowed and decided by this side). */
static uint32_t select_extras(Cpu *c, gaddr me, uint32_t ch) {
    if (ch == CH_AKUMA || ch == CH_BALROG_2) {
        uint32_t to = ch == CH_AKUMA ? CH_SHIN_AKUMA : CH_BALROG;
        if (S8(GAME + G_NET) == 0 || CALL1(f_0889472C, to) == 0) {
            *B(me + F_CHARACTER) = (uint8_t)to;
            return to;
        }
        return ch;
    }
    if (ch != CH_BISON) return ch;
    int8_t mode = S8(GAME + G_MODE);
    if (mode != 1 && mode != 0 && mode != 0x12 && mode != 0x11 && mode != 9) return ch;
    if (S8(GAME + G_NET) != 0) {
        if (CALL1(f_0889472C, CH_FINAL_BISON) != 0) return ch;
        if (S8(BATTLE + BT_P2_FIRST) != 0) return ch;
    }
    *B(me + F_FINAL_BISON) = 1;
    *B(me + F_ISM) = 0;
    return ch;
}

/* Chosen (0x08897344, a2 the button): the player plays as an entry (copied in, with its ISM and
   colours) or as himself (his entry cleared, SELECT's extras); a new character clears + 400 / 404 /
   411; the colours by the button; and, if the ISM is decided, it is now. */
static void character_chosen(Cpu *c, gaddr me, gaddr other, uint32_t buttons) {
    buttons &= 0xFFFF;
    next_step(me);
    set_flag8(me + F_CHOSEN, CHOSE_CHARACTER);
    CALL1(f_08896AA4, me);   /* the result is not used */
    uint32_t ch = (uint16_t)(int32_t)S8(me + F_CHARACTER);
    int8_t entry = S8(me + F_ENTRY_BOX);
    *B(me + F_FINAL_BISON) = 0;
    gaddr mine = ENTRIES + (uint32_t)(int32_t)S8(me + F_SLOT) * ENTRY_SIZE;
    if (entry == 0) {
        for (uint32_t i = 0; i < ENTRY_SIZE; i++) *B(mine + i) = 0;
        uint32_t held = CALL1(f_08896B18, me);
        int extras = 1;
        if (CALL0(f_088D5E44) == 0) extras = S16(SYS + SYS_UNLOCKS) & 1;
        if ((held & BUTTON_SELECT) && extras) ch = select_extras(c, me, ch);
    } else {
        int16_t k = (int16_t)(entry - 1);
        gaddr from;
        if (CALL0(f_088D7C1C) != 0) {
            int32_t n = k >> 1;
            from = NET_ENTRIES + (uint32_t)(int32_t)S8(me + F_SLOT) * 208u + (uint32_t)(n * 60);
            CALL3(f_089E0DC4, mine, from, NET_ENTRY_SIZE);
            *B(me + F_ISM) = *B(from + 2);
            *B(me + F_NET_ENTRY) = (uint8_t)(n + 1);
        } else {
            from = SAVED_ENTRIES + (uint32_t)(int32_t)k * ENTRY_SIZE;
            for (uint32_t i = 0; i < ENTRY_SIZE; i += 4) *W(mine + i) = *W(from + i);
            *B(me + F_ISM) = *B(from + 2);
        }
        *B(me + F_COLOUR) = *B(from + 3);
        *B(me + F_COLOUR_COPY) = *B(from + 3);
        *B(GAME + G_NET_ENTRY) = (uint8_t)k;
    }

    int32_t last = S8(me + F_LAST_CHARACTER);
    int same = (ch == CH_CHUNLI && last == CH_CHUNLI_X) || (ch == CH_SODOM && last == CH_SODOM_X) ||
               (ch == CH_GEN && last == CH_GEN_X) || ch == (uint32_t)last;
    if (!same) {
        *B(me + 411) = 0;
        *H(me + F_KEPT_400 + 4) = 0;
        *W(me + F_KEPT_400) = 0;
    }
    *B(me + F_LAST_CHARACTER) = (uint8_t)ch;
    if (S8(me + F_ENTRY_BOX) == 0) {
        uint8_t colour = colour_of(buttons);
        *B(me + F_COLOUR_COPY) = colour;
        *B(me + F_COLOUR) = colour;
        avoid_same_colours(me, other, 1);
    }
    *H(me + F_TIMER) = WAIT_SHOWN;

    if (S8(GAME + G_NET) != 0 && S8(GAME + G_NET_LOBBY) >= 2) {
        set_flag8(me + F_CHOSEN, CHOSE_ISM);
        uint8_t ism = (uint8_t)(S8(GAME + G_NET_LOBBY) - 3);
        *B(me + F_ISM) = ism;
        *B(me + F_LAST_ISM) = ism;
        CALL2(f_08895564, me, other);
    }
    if (ism_decided(me)) decide_ism(c, me, other);
}

/* Wait (0x08894990): the character shown a while; then done, the box to come back to kept, the
   style done if only one is offered (or Final Bison), the ISM if decided. */
static void character_wait(Cpu *c, gaddr me, gaddr other) {
    if (!timer_out(me)) return;
    next_state(me);
    set_flag16(me + F_DONE, DONE_CHARACTER);
    int8_t entry = S8(me + F_ENTRY_BOX);
    if (entry != 0) {
        *B(me + F_BOX) = (uint8_t)(entry + 0x2D);
    } else {
        if (S8(me + F_FINAL_BISON) != 0) {
            set_flag16(me + F_DONE, DONE_STYLE | DONE_ISM);
            CALL2(f_08895564, me, other);
        } else if (CALL1(f_088A4F1C, me) == 1) {
            set_flag16(me + F_DONE, DONE_STYLE);
        }
        *B(me + F_BOX) = 0;
        int8_t box = S8(me + F_RANDOM_BOX);
        if (box != 0) *B(me + F_BOX) = (uint8_t)(S8(me + F_RANDOM_HIGH) | box);
    }
    if (S8(me + F_CHOSEN) & CHOSE_ISM) set_flag16(me + F_DONE, DONE_ISM);
}

/* ---- states 1 and 2: the style and the ISM ---- */

/* Enter (style 0x08894FC0, ISM 0x08894C74): with BATTLE + 90, the other player's choice if made;
   then, if done, the next state at once; otherwise the choose step (the ISM's with a sound). */
static void choice_enter(Cpu *c, gaddr me, gaddr other, uint16_t bit, uint32_t field,
                         void (*chosen)(Cpu *), void (*next)(Cpu *), int sound) {
    if (S8(BATTLE + BT_P2_FIRST) != 0 && (S8(other + F_CHOSEN) & bit)) {
        set_flag16(me + F_DONE, bit);
        *B(me + field) = *B(other + field);
        CALL2(chosen, me, other);
    }
    if (S16(me + F_DONE) & bit) {
        next_state(me);
        *H(me + F_TIMER) = WAIT_CHOICE;
        CALL2(next, me, other);
        return;
    }
    *H(me + F_TIMER) = WAIT_CHOICE;
    next_step(me);
    if (sound) CALL3(f_0887C4CC, 0, SOUND_ISM, 0xC0);
}

/* The play styles the mode offers (0x088A4F1C): a bit per style 0-4 (style 4 is never offered by
   the select screen, 0x0889507C). */
static uint32_t styles_offered(gaddr me) {
    int8_t mode = S8(GAME + G_MODE);
    uint32_t m = (uint8_t)mode;
    int8_t free_kind = S8(BATTLE + 159);
    if (mode == 6 || (mode == 16 && free_kind == 2)) return 0xD;
    if (m == 1) return S8(me + F_FINAL_BISON) != 0 ? 0x11 : 0x1F;
    if (m == 2) return S8(me + F_FINAL_BISON) != 0 ? 0x01 : 0x0F;
    if (m == 3 || m == 8 || m == 15 || m == 13 || m == 19 || m == 14) return 1;
    if (mode == 7 && (S8(BATTLE + 79) == 4 || S8(BATTLE + 79) == 5)) return 1;
    if (mode == 16 && (free_kind == 4 || free_kind == 3)) return 1;
    return 0xF;
}

/* The style chosen (0x088951DC): each style's flag. */
static void style_chosen(gaddr me) {
    next_step(me);
    int8_t s = S8(me + F_STYLE);
    if (s == 4) *B(me + F_STYLE_4_ON) = 0xFF;
    if (s == 1) *B(me + F_STYLE_1_ON) = 1;
    if (s == 2) *B(me + F_STYLE_2_ON) = 1;
    if (s == 3) { *B(me + F_ISM_FIXED) = 1; *B(me + F_ISM) = 0xFF; }
    set_flag8(me + F_CHOSEN, CHOSE_STYLE);
    *H(me + F_TIMER) = WAIT_SHOWN;
}

/* Choose the style (0x0889507C): an attack button, or with BATTLE + 90 at once, chooses; up / down
   step through the styles offered (never style 4). */
static void style_choose(Cpu *c, gaddr me, gaddr other) {
    if (CALL0(f_088D5E44) != 0 && timer_out(me)) { CALL2(f_088951DC, me, other); return; }
    if (S8(BATTLE + BT_P2_FIRST) != 0) { CALL2(f_088951DC, me, other); return; }
    uint32_t pressed = CALL1(f_08896AA4, me);
    if (pressed & BUTTONS_CHOOSE) { CALL2(f_088951DC, me, other); return; }
    uint32_t offered = CALL1(f_088A4F1C, me) & 0xEF;
    int16_t s = S8(me + F_STYLE);
    if (pressed & DIR_UP) {
        do { int16_t was = s; s = (int16_t)(s - 1); if (was == 0) s = 4; } while (!(offered & (1u << (s & 31))));
    } else if (pressed & DIR_DOWN) {
        do { int16_t was = s; s = (int16_t)(s + 1); if (was == 4) s = 0; } while (!(offered & (1u << (s & 31))));
    }
    *B(me + F_STYLE) = (uint8_t)s;
}

/* Wait for the style (0x08895248): then the ISM, decided now if it is to be. */
static void style_wait(Cpu *c, gaddr me, gaddr other) {
    if (!timer_out(me)) return;
    next_state(me);
    set_flag16(me + F_DONE, DONE_STYLE);
    if (!ism_decided(me)) return;
    set_flag8(me + F_CHOSEN, CHOSE_ISM);
    set_flag16(me + F_DONE, DONE_ISM);
    *B(me + F_LAST_ISM) = *B(me + F_ISM);
    CALL2(f_08895564, me, other);
}

/* The ISM chosen (0x08894E5C): the X-ISM character for it, and the colours. */
static void ism_chosen(gaddr me, gaddr other) {
    next_step(me);
    int8_t ism = S8(me + F_ISM);
    *B(me + F_LAST_ISM) = (uint8_t)ism;
    int32_t to = x_ism_character(S8(me + F_CHARACTER), ism);
    if (to >= 0) {
        *B(me + F_LAST_CHARACTER) = (uint8_t)to;
        *B(me + F_CHARACTER) = (uint8_t)to;
    }
    avoid_same_colours(me, other, 1);
    set_flag8(me + F_CHOSEN, CHOSE_ISM);
    *H(me + F_TIMER) = WAIT_SHOWN;
}

/* An ISM decided for the player (0x08895564): the same, without the step and the timer. */
static void ism_decided_apply(gaddr me, gaddr other) {
    int32_t to = x_ism_character(S8(me + F_CHARACTER), S8(me + F_ISM));
    if (to >= 0) {
        *B(me + F_LAST_CHARACTER) = (uint8_t)to;
        *B(me + F_CHARACTER) = (uint8_t)to;
    }
    avoid_same_colours(me, other, 0);
}

/* Choose the ISM (0x08894D40): an attack button chooses; up / down go round -1, 0, 1. A network
   battle chooses when time runs out (with BATTLE + 90, the other's ISM if made). */
static void ism_choose(Cpu *c, gaddr me, gaddr other) {
    if (CALL0(f_088D5E44) != 0 && timer_out(me)) {
        if (S8(BATTLE + BT_P2_FIRST) != 0 && (S8(other + F_CHOSEN) & CHOSE_ISM))
            *B(me + F_ISM) = *B(other + F_ISM);
        CALL2(f_08894E5C, me, other);
        return;
    }
    uint32_t pressed = CALL1(f_08896AA4, me);
    if (pressed & BUTTONS_CHOOSE) { CALL2(f_08894E5C, me, other); return; }
    int16_t v = S8(me + F_ISM);
    if (pressed & DIR_UP)   { v = (int16_t)(v - 1); if (v < -1) v = 1; }
    if (pressed & DIR_DOWN) { v = (int16_t)(v + 1); if (v >= 2) v = -1; }
    *B(me + F_ISM) = (uint8_t)v;
}

/* Wait (ISM 0x08894F4C, speed 0x088954A0): then the next state, this part done. */
static void choice_wait(gaddr me, uint16_t bit) {
    if (!timer_out(me)) return;
    next_state(me);
    set_flag16(me + F_DONE, bit);
}

/* ---- state 3: the speed ---- */

/* Enter (0x08895338): done, or already shown (GAME + 20's high byte; the first player to get here
   shows it, so a second one is through at once): state 4. Otherwise shown, and the choose step. */
static void speed_enter(Cpu *c, gaddr me, gaddr other) {
    uint16_t done = *H(me + F_DONE);
    if ((done & DONE_SPEED) || (*H(GAME + G_SPEED) & 0xFF00)) {
        next_state(me);
        *H(me + F_DONE) = (uint16_t)(done | DONE_SPEED);
        CALL2(f_088954E0, me, other);
        return;
    }
    next_step(me);
    set_flag16(GAME + G_SPEED, 0x101);
    *H(me + F_TIMER) = WAIT_CHOICE;
}

/* Choose the speed (0x088953CC): up / down switch the two choices; an attack button (or, network,
   time) chooses: BATTLE + 22 from SPEEDS. */
static void speed_choose(Cpu *c, gaddr me) {
    uint32_t pressed = CALL1(f_08896AA4, me);
    int chosen = CALL0(f_088D7C1C) != 0 && timer_out(me);
    if (chosen || (pressed & BUTTONS_CHOOSE)) {
        next_step(me);
        set_flag8(me + F_CHOSEN, CHOSE_SPEED);
        *H(me + F_TIMER) = WAIT_SHOWN;
        *B(BATTLE + BT_SPEED) = *B(SPEEDS - 1 + (uint32_t)((*H(GAME + G_SPEED) & 0xFF) + S8(GAME + G_SPEED_BASE)));
    } else if (pressed & (DIR_DOWN | DIR_UP)) {
        *H(GAME + G_SPEED) = (uint16_t)(*H(GAME + G_SPEED) ^ 1);
    }
}

/* ---- state 4 ---- */

/* The choice counted (0x088969FC), by character (the X-ISM ones as their own) and ISM (an entry
   counts as ISM 2). The count was meant to stop at 0xFFFF, but the original compares the
   sign-extended count with 0xFFFF, so it never stops: it wraps to 0. */
static void count_choice(gaddr me) {
    int32_t ch = S8(me + F_CHARACTER), ism = S8(me + F_ISM);
    if (ch == CH_CHUNLI_X) ch = CH_CHUNLI;
    if (ch == CH_GEN_X) ch = CH_GEN;
    if (ch == CH_SODOM_X) ch = CH_SODOM;
    if (S8(ENTRIES + (uint32_t)(int32_t)S8(me + F_SLOT) * ENTRY_SIZE) != 0) ism = 2;
    gaddr n = SYS + SYS_CHOICES + (uint32_t)(ch * 8 + ism * 2);
    *H(n) = (uint16_t)(*H(n) + 1);
}

SFA_FUNC_WRAPPED(08896AA4, game_select_pad_pressed, selectscreen_pressed) { RETURN(box_pad(ARG(0), PAD_PRESSED)); }
SFA_FUNC_WRAPPED(08896B18, game_select_pad_held, selectscreen_held)          { RETURN(box_pad(ARG(0), PAD_HELD)); }
SFA_FUNC(0889472C, game_select_barred)          { RETURN(character_barred(ARG(0))); }
SFA_FUNC(08894644, game_select_character_enter) { SELECT_FRAME(0x20, character_enter(c, ARG(0), ARG(1))); }
SFA_FUNC(08894768, game_select_character_move)  { SELECT_FRAME(0x30, character_move(c, ARG(0), ARG(1))); }
SFA_FUNC(08897008, game_select_character_grid)  { SELECT_FRAME(0x50, character_grid(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(08897344, game_select_character_chosen) { SELECT_FRAME(0x40, character_chosen(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(08894990, game_select_character_wait)  { SELECT_FRAME(0x20, character_wait(c, ARG(0), ARG(1))); }
SFA_FUNC(08894FC0, game_select_style_enter)     { SELECT_FRAME(0x20, choice_enter(c, ARG(0), ARG(1), DONE_STYLE, F_STYLE, f_088951DC, f_08894C40, 0)); }
SFA_FUNC(088A4F1C, game_select_styles_offered)  { RETURN(styles_offered(ARG(0))); }
SFA_FUNC(0889507C, game_select_style_choose)    { SELECT_FRAME(0x20, style_choose(c, ARG(0), ARG(1))); }
SFA_FUNC(088951DC, game_select_style_chosen)    { style_chosen(ARG(0)); }
SFA_FUNC(08895248, game_select_style_wait)      { SELECT_FRAME(0x20, style_wait(c, ARG(0), ARG(1))); }
SFA_FUNC(08894C74, game_select_ism_enter)       { SELECT_FRAME(0x20, choice_enter(c, ARG(0), ARG(1), DONE_ISM, F_ISM, f_08894E5C, f_08895304, 1)); }
SFA_FUNC(08894D40, game_select_ism_choose)      { SELECT_FRAME(0x20, ism_choose(c, ARG(0), ARG(1))); }
SFA_FUNC(08894E5C, game_select_ism_chosen)      { ism_chosen(ARG(0), ARG(1)); }
SFA_FUNC(08895564, game_select_ism_decided)     { ism_decided_apply(ARG(0), ARG(1)); }
SFA_FUNC(08894A84, game_select_random)          { int32_t ch; SELECT_FRAME(0x20, ch = random_character(c, ARG(0), ARG(1))); RETURN((uint32_t)ch); }
SFA_FUNC(08894F4C, game_select_ism_wait)        { choice_wait(ARG(0), DONE_ISM); }
SFA_FUNC(08895338, game_select_speed_enter)     { SELECT_FRAME(0x20, speed_enter(c, ARG(0), ARG(1))); }
SFA_FUNC(088953CC, game_select_speed_choose)    { SELECT_FRAME(0x20, speed_choose(c, ARG(0))); }
SFA_FUNC(088954A0, game_select_speed_wait)      { choice_wait(ARG(0), DONE_SPEED); }
SFA_FUNC(088969FC, game_select_count_choice)    { count_choice(ARG(0)); }
