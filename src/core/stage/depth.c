/* Draw depths: how far forward each layer of a screen is drawn (a larger depth is in front). */
#include "../decomp.h"

#define DEPTHS 0x08A03B10u    /* u16 pairs: [column][row] */

/* The depth in the table's `row` (0 or 1) of `column`. */
SFA_FUNC(0880A3C4, draw_depth) {
    uint32_t row = ARG(0), column = ARG(1);
    RETURN(*(uint16_t *)guest_ptr(DEPTHS + (column * 2u + row) * 2u));
}
