/* Small questions the rest of the game asks about the network (ad-hoc VS) state.
 *
 * The network task keeps its state at 0x09523980 (docs/PSP_PROGRAM_MAP.md); offline, all of it is 0
 * and these answer "no".
 */
#include "../decomp.h"
#include "../psp_os.h"

#define GAME        0x08E75910u
#define G_MODE      114            /* s8: the mode; 12 is NETWORK */
#define MODE_NETWORK 12
#define NET         0x09523980u    /* the network task's state */
#define N_FAILED    8              /* u32: the session's failure, 0 while it is fine */
#define N_BATTLE    34             /* u8: a network battle is on outside mode 12 */
#define N_STARTED   42             /* u8: 1 once the session has started */
#define N_ROLE      43             /* u8: 2 or 4 while waiting for the host's choice */
#define N_FLAG_59   59             /* s8 */
#define N_TEXT_FLAG 60             /* u8: the network text's flag, cleared at the end of every frame */
#define N_FLAG_61   61             /* u8 */
#define N_FLAG_267  267            /* s8 */
#define NET_STEP    0x09523960u    /* the network step's state */
#define NS_FLAG_25  25             /* u8 */

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline int8_t S8(gaddr a) { return (int8_t)*B(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* A network battle: the mode is NETWORK, or the state says one is on. */
static int network_battle(void) { return S8(GAME + G_MODE) == MODE_NETWORK || *B(NET + N_BATTLE) != 0; }

/* A network battle whose session has started. */
SFA_FUNC(088D7CE0, net_session_started) { RETURN(network_battle() && *B(NET + N_STARTED) == 1); }

/* A network battle in which this side waits for the host's choice (role 2 or 4). */
SFA_FUNC(088D5E7C, net_waits_for_host) {
    RETURN(network_battle() && (*B(NET + N_ROLE) == 2 || *B(NET + N_ROLE) == 4));
}

SFA_FUNC(088D7E80, net_flag_59) { RETURN((uint32_t)S8(NET + N_FLAG_59)); }

/* The end of a frame clears the network text's flag (the frame's last steps, object/frame.c). */
SFA_FUNC(088D7E90, net_clear_text_flag) { *B(NET + N_TEXT_FLAG) = 0; }

/* The session's failure code, 0 while it is fine. */
SFA_FUNC(088D7F18, net_session_failed) { RETURN(*W(NET + N_FAILED)); }

/* Passes on +267, and sets +61 when it is set (asked once a frame by 0x0880AA74). */
SFA_FUNC(088D7F28, net_flag_267) {
    int8_t v = S8(NET + N_FLAG_267);
    if (v != 0) *B(NET + N_FLAG_61) = 1;
    RETURN((uint32_t)v);
}

SFA_FUNC(088E3FE0, net_step_clear_25) { *B(NET_STEP + NS_FLAG_25) = 0; }

/* ---- 0.69.0 ---- */

void f_089F3470(Cpu *c);   /* memset */

SFA_FUNC(088D55D0, net_clear) { c->r[29] -= 0x20; CALL3(f_089F3470, NET, 0, 0x10C); c->r[29] += 0x20; }

/* A network battle in which this side waits for the host (role 2 or 4) and +36 is set. */
SFA_FUNC(088D5ECC, net_waits_and_36) {
    RETURN(network_battle() && (*B(NET + N_ROLE) == 2 || *B(NET + N_ROLE) == 4) && S8(NET + 36) != 0);
}

/* A network battle with +35 set. */
SFA_FUNC(088D7C4C, net_battle_and_35) { RETURN(network_battle() && *B(NET + 35) != 0); }

/* +264 up by one (argument not 0) or down by one, never under 0. */
SFA_FUNC(088D7EA0, net_count_264) {
    int8_t v = (int8_t)(S8(NET + 264) + ((int8_t)ARG(0) != 0 ? 1 : -1));
    *B(NET + 264) = (uint8_t)(v < 0 ? 0 : v);
}

/* The 64 bytes at 0x09523C20 and the word at 0x08B02EA8 cleared. */
SFA_FUNC(088DD81C, net_clear_09523C20) {
    c->r[29] -= 0x20;
    CALL3(f_089F3470, 0x09523C20u, 0, 0x40);
    *W(0x08B02EA8u) = 0;
    c->r[29] += 0x20;
}

/* The 64 KB block 0x08B02EA0 points at is cleared and given its header: a mark (0x1AE49CF0) and where
   it ends. */
SFA_FUNC(088DC080, net_block_reset) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    gaddr block = *W(0x08B02EA0u);
    c->r[29] -= 0x20;
    c->r[16] = 0x08B00000u; c->r[17] = block; c->r[18] = 0x10000;
    CALL3(f_089F3470, block, 0, 0x10000);
    *W(block) = 0x1AE49CF0u;
    *W(block + 4) = *W(0x08B02EA0u) + 0x10000;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* ---- 0.74.0: the session (0x09523920, task/fight_network.c) and its thread's record ---- */


#define SESSION      0x09523920u   /* the session: +16 its error, +37 / +48 up, +38 a counter, +49 halted */
#define SESSION_SIZE 0x34
#define NET_STEP_SIZE 0x1C
#define NET_THREAD   0x095236C0u   /* +4 a thread to wake, +16 whether to; 0x20 bytes */
#define NET_SEMA     0x08A3EB88u   /* u32: the semaphore "network_sema", once made */
#define NET_SEMA_NAME 0x08A01380u  /* "network_sema" */


SFA_FUNC(088E3294, net_session_clear)   { c->r[29] -= 0x20; CALL3(f_089F3470, SESSION, 0, SESSION_SIZE); c->r[29] += 0x20; }
SFA_FUNC(088E3EB0, net_step_clear)      { c->r[29] -= 0x20; CALL3(f_089F3470, NET_STEP, 0, NET_STEP_SIZE); c->r[29] += 0x20; }

/* 1 while the session has no error and has not halted the fight (+16, +49). */
SFA_FUNC(088E358C, net_session_fine)    { RETURN(*W(SESSION + 16) == 0 && S8(SESSION + 49) == 0); }

/* 1 while the session is up neither way (+48, +37). */
SFA_FUNC(088E3604, net_session_down)    { RETURN(S8(SESSION + 48) == 0 && S8(SESSION + 37) == 0); }

/* While the session is up (+48), +38 counts 0 - 3 round. */
SFA_FUNC(088E36B8, net_session_count_38) {
    if (S8(SESSION + 48) == 0) return;
    *B(SESSION + 38) = (uint8_t)((S8(SESSION + 38) + 1) & 3);
}

/* The buttons this frame (the button-config remap): while the session is up and has not failed they
   go into the session's first word and the answer is 0; otherwise they are answered as they are. */
SFA_FUNC(088E3660, net_take_buttons) {
    uint32_t buttons = ARG(0);
    if (S8(SESSION + 48) == 0) { RETURN(buttons); return; }
    c->r[29] -= 0x10;
    c->r[4] = buttons; c->r[5] = SESSION;
    f_088D7F18(c);
    uint32_t failed = c->r[2];
    c->r[29] += 0x10;
    if (failed != 0) { RETURN(buttons); return; }
    *W(SESSION) = buttons;
    RETURN(0);
}

/* Wakes the network's thread (+4) when +16 asks for it. */
SFA_FUNC(088E1ABC, net_wake_thread) {
    if (S8(NET_THREAD + 16) == 0 || *W(NET_THREAD + 4) == 0) return;
    c->r[29] -= 0x20;
    os_wakeup_thread(c, (int32_t)*W(NET_THREAD + 4));
    c->r[29] += 0x20;
}

/* The thread's record cleared, and the semaphore "network_sema" (count 1 of 1) made the first time. */
SFA_FUNC(088E0A58, net_thread_reset) {
    c->r[29] -= 0x30;
    CALL3(f_089F3470, NET_THREAD, 0, 0x20);
    if (*W(NET_SEMA) == 0) {
        int32_t id = os_create_sema(c, NET_SEMA_NAME, 0, 1, 1, 0);
        if (id > 0) *W(NET_SEMA) = (uint32_t)id;
    }
    c->r[29] += 0x30;
}

void f_088DC080(Cpu *c);   /* net_block_reset */
void f_088DEB10(Cpu *c);   /* record_23740_reset (misc.c) */
void f_088DD81C(Cpu *c);   /* net_clear_09523C20 */

#define NET_BLOCK_PTR  0x08B02EA0u   /* u32: the 64 KB block (net_block_reset) */
#define NET_HEAP_SEMA  0x08B02EA4u   /* u32: the semaphore "heap_work_sema" */

/* The network's state from scratch: the 64 KB block at 0x09316A00 (and the semaphore
   "heap_work_sema", count 1 of 1), the thread's record, 0x09523740, the step, the session, the 0xF4
   bytes at 0x09523820, the text and 0x09523C20. */
SFA_FUNC(088DBF88, net_reset_all) {
    c->r[29] -= 0x30;
    *W(NET_BLOCK_PTR) = 0x09316A00u;
    *W(NET_HEAP_SEMA) = (uint32_t)os_create_sema(c, 0x08A012B8u, 0, 1, 1, 0);
    c->r[4] = 0x08B00000u;
    f_088DC080(c);
    f_088E0A58(c);
    f_088DEB10(c);
    f_088E3EB0(c);
    f_088E3294(c);
    CALL3(f_089F3470, 0x09523820u, 0, 0xF4);
    f_088D55D0(c);
    f_088DD81C(c);
    c->r[29] += 0x30;
}
