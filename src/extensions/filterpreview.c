/* The FILTER screen's live preview (filterscreen.c): to come. */
#include "rt.h"
void f_0887C4CC(Cpu *c);
void filterpreview_start(Cpu *c, int f) {
    (void)f;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 0; c->r[5] = 0x2F; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
