/* Small questions the rest of the game asks about the network (ad-hoc VS) state.
 *
 * The network task keeps its state at 0x09523980 (docs/PSP_PROGRAM_MAP.md); offline, all of it is 0
 * and these answer "no".
 */
#include "../decomp.h"

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
