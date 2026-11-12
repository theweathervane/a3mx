/* The text queue: every piece of text a screen wants drawn this frame.
 *
 * Screens ask for text with 0x0880A6CC; the two text passes at the end of the frame (0x0880B098,
 * 0x08812520) draw what was asked for and empty the queue.
 */
#include "../decomp.h"

#define QUEUE       0x092DF240u
#define QUEUE_SIZE  96

typedef struct {
    uint8_t  used;        /* +0: 1 while waiting to be drawn */
    uint8_t  font;        /* +1 */
    uint8_t  colour;      /* +2 */
    uint8_t  kind;        /* +3: 7 for text asked for this way */
    int16_t  x, y;        /* +4, +6 */
    gaddr    string;      /* +8 */
    uint8_t  drawn;       /* +12: cleared here */
    uint8_t  pad[3];
} TextRequest;
GUEST_STRUCT_SIZE(TextRequest, 16);

/* Takes the first free entry; returns its address, or 0 when all 96 are taken (the text is dropped). */
static gaddr text_request(gaddr string, uint8_t font, int16_t x, int16_t y, uint8_t colour) {
    TextRequest *q = guest_ptr(QUEUE);
    for (int i = 0; i < QUEUE_SIZE; i++) {
        if (q[i].used != 0) continue;
        q[i].used = 1;
        q[i].kind = 7;
        q[i].font = font;
        q[i].x = x;
        q[i].y = y;
        q[i].colour = colour;
        q[i].string = string;
        q[i].drawn = 0;
        return guest_addr(&q[i]);
    }
    return 0;
}

/* text(string, font, x, y, colour) */
SFA_FUNC(0880A6CC, game_text) {
    RETURN(text_request(ARG(0), (uint8_t)ARG(1), (int16_t)ARG(2), (int16_t)ARG(3), (uint8_t)ARG(4)));
}
