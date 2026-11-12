/* The network's part of every fight frame (run by the fight loop, fight.c, even offline):
 *
 *   0x088E30FC  at a frame's start: the session's packets handled, and the fight halted
 *               (GAME + 0x59 and + 0x91) while the session waits or has failed
 *   0x088DBC7C  the network text's steps (a message box: shown, counted down, answered, closed)
 *
 * Offline both return almost at once; the rest is written from the program as it stands.
 */
#include "../decomp.h"

void rt_call_indirect(Cpu *c, uint32_t addr);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

#define NET            0x09523920u   /* the session */
#define NET_TEXT       0x09523980u   /* the network text */
#define PACKET_HANDLERS 0x08A3EBB8u  /* by packet + 212: (session, packet) -> handled */
#define NET_TEXT_DONE  0x08A3EB0Cu   /* what the text points at once closed */
#define NET_ERROR_TIMEOUT 0x81001005u
#define NET_TIMEOUT    1200          /* frames without word before the session fails */
#define NET_PREF       0x08E7606Cu   /* u8: nonzero = the text counts down */
#define PAD_1_PRESSED  0x08E75BACu
#define TEXT_FADE      0x08E75B9Cu   /* float */

/* GAME fields */
#define G_HALT   0x59
#define G_HALT_3 0x91
#define G_BUSY   93

void f_088D7F18(Cpu *c);   /* the session has failed */
void f_088DD3DC(Cpu *c);   /* the first received packet */
void f_088DD418(Cpu *c);   /* the next received packet (a0 the cursor's address) */
void f_088E30DC(Cpu *c);   /* (session) */
void f_088E307C(Cpu *c);   /* (session): packets done */
void f_08809BE4(Cpu *c);   /* (n) */
void f_0880ABD4(Cpu *c); void f_088099D4(Cpu *c); void f_0880AA74(Cpu *c);
void f_088E3F98(Cpu *c);   /* (on) */
void f_088E3F44(Cpu *c);   /* -> the text has been answered */
void f_0887C4CC(Cpu *c);   /* a sound (a0, a1, a2) */
void f_088E35B8(Cpu *c);
void f_088D7F48(Cpu *c);   /* (game) */
void f_088D6D3C(Cpu *c);   /* draws the text (text) */

/* No word for NET_TIMEOUT frames: the session fails once (its error set if none yet). */
static void session_timeout(void) {
    if (*W(NET + 16) == 0) *W(NET + 16) = NET_ERROR_TIMEOUT;
}

/* Hands every received packet to its handler; 1 if all of them were handled. The cursor lives on
   the stack at frame + 16, as the original passes its address on. */
static int handle_packets(Cpu *c, gaddr frame) {
    int all = 1;
    gaddr cursor = frame + 16;
    f_088DD3DC(c);
    *W(cursor) = c->r[2];
    for (;;) {
        c->r[4] = cursor; f_088DD418(c);
        gaddr packet = c->r[2];
        if (packet == 0) break;
        gaddr fn = *W(PACKET_HANDLERS + *H(packet + 212) * 4u);
        c->r[4] = NET; c->r[5] = packet; c->r[6] = fn;
        rt_call_indirect(c, fn);
        all &= (int)c->r[2];
    }
    return all;
}

static void frame_start(Cpu *c, gaddr g, gaddr frame) {
    if (CALL0(f_088D7F18) != 0) {
        *B(NET + 49) = 1;
    } else if (S8(NET + 48) != 0) {
        *B(NET + 44) = 0;
        *B(g + G_HALT) = 0;
        *B(g + G_HALT_3) = 0;
        if (S16(NET + 30) > NET_TIMEOUT) {
            session_timeout();
        } else if (!handle_packets(c, frame)) {
            *B(NET + 49) = 1;
            CALL1(f_088E307C, NET);
        } else {
            CALL1(f_088E30DC, NET);
            if (S8(NET + 37) != 0) *H(NET + 30) = 0;
            else CALL1(f_088E307C, NET);
        }
    } else if (S8(NET + 37) == 0) {
        *B(NET + 49) = 0;
    } else {
        CALL1(f_088E30DC, NET);
        if (S16(NET + 30) > NET_TIMEOUT) session_timeout();
        else CALL1(f_088E307C, NET);
    }
    if (S8(NET + 49) != 0) {
        *B(g + G_HALT) = 1;
        *B(g + G_HALT_3) = 1;
    }
}

/* ---- the network text ---- */

enum { TX_STEP = 0, TX_TIMER = 4, TX_SHOWN = 59, TX_BUSY = 61, TX_COUNTS = 264, TX_OPEN = 267 };

static void next_step(gaddr t) { *H(t + TX_STEP) = (uint16_t)(*H(t + TX_STEP) + 1); }

/* Counts the timer down; 1 when it reaches 0 */
static int timer_done(gaddr t) {
    int16_t left = (int16_t)(S16(t + TX_TIMER) - 1);
    *H(t + TX_TIMER) = (uint16_t)left;
    return left == 0;
}

static void text_steps(Cpu *c, gaddr g) {
    gaddr t = NET_TEXT;
    uint16_t step = *H(t + TX_STEP);
    switch (step) {
    default:   /* 0 (or anything unknown): open it */
        if (*W(NET + 20) != 0) {
            *B(*W(NET + 20)) = 0;
            *W(NET + 20) = 0;
        }
        CALL1(f_08809BE4, 0x3C);
        *H(t + TX_TIMER) = 0x3C;
        *H(t + TX_STEP) = 1;
        *B(t + TX_SHOWN) = 1;
        *B(t + TX_BUSY) = 0;
        *B(t + TX_OPEN) = 1;
        /* fall through: step 1 runs the same frame */
    case 1:
        if (S8(g + G_BUSY) != 0) break;
        if (!FCMP(F(TEXT_FADE), 0.0f, 2)) {
            *B(t + TX_BUSY) = 1;
            c->r[4] = 1;   /* as the original leaves it */
            f_0880ABD4(c);
        }
        CALL0(f_088099D4);
        next_step(t);
        break;
    case 2:
        CALL1(f_088E3F98, 0);
        if (S8(t + TX_COUNTS) != 0 || *B(NET_PREF) == 0) { *H(t + TX_TIMER) = 0x78; break; }
        if (timer_done(t)) {
            *H(t + TX_TIMER) = 0x4B0;
            next_step(t);
        }
        break;
    case 3:
        if (S8(t + TX_COUNTS) != 0 || *B(NET_PREF) == 0) {
            CALL1(f_088E3F98, 0);
            *H(t + TX_TIMER) = 0x78;
            *H(t + TX_STEP) = 2;
            break;
        }
        CALL1(f_088E3F98, 1);
        if ((*W(PAD_1_PRESSED) & 0x100) == 0 && S8(t + 33) == 0 && !timer_done(t)) break;
        CALL3(f_0887C4CC, 0, 0x2F, 0xC0);
        next_step(t);
        break;
    case 4:
        if (CALL0(f_088E3F44) == 0) break;
        *B(t + TX_OPEN) = 0;
        CALL0(f_0880AA74);
        next_step(t);
        break;
    case 5:
        if (S8(g + G_BUSY) != 0) break;
        *B(t + TX_BUSY) = 0;
        next_step(t);
        break;
    case 6:
        *B(t + TX_SHOWN) = 0;
        *B(t + TX_BUSY) = 0;
        CALL0(f_088E35B8);
        *W(t + 20) = NET_TEXT_DONE;
        *H(t + 24) = 0;
        *H(t + 2) = 0;
        *H(t + TX_STEP) = 0;
        *B(t + 47) = 0;
        CALL1(f_088D7F48, g);
        break;
    }
    CALL1(f_088D6D3C, t);
}

/* Nothing while there is no text (+8) or the frame is skipped (BATTLE + 23) */
static void network_text(Cpu *c, gaddr g, gaddr battle) {
    if (*W(NET_TEXT + 8) == 0 || S8(battle + 23) != 0) return;
    text_steps(c, g);
}

SFA_FUNC(088E30FC, game_net_frame_start) { c->r[29] -= 0x30; frame_start(c, ARG(0), c->r[29]); c->r[29] += 0x30; }
SFA_FUNC(088DBC7C, game_net_text_steps)  { c->r[29] -= 0x20; network_text(c, ARG(0), ARG(1)); c->r[29] += 0x20; }
