/* One routine of the link play's course: the next course picked and walked down to where it
   ends. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LINK 0x09517A78u /* the link play's state: +852 the course's table of stages, +856 its
                            number, +858 a bit for each course used up, +860 the stage reached,
                            +864 a bit for each stage that follows it, +868.. a word of bits for
                            each course saying which stages are open, +892 the stage's number */

/* (a, b): the first of the six courses not used up is taken (number 6 when all are), its table
   (from the list at 0x08A11DDC) noted, and from the first stage (0x08A12068) the walk goes down:
   at each stage (its three ways on at +8, +12 and +16, below 0 for none) the first way whose
   stage is open in the course's word is followed. A stage whose first two ways are both closed
   off uses the course up, and the whole pick starts again. Where no way is open the walk ends:
   the stage's number and place are kept, with a bit for each way on from it. A 0x20 frame. */
SFA_FUNC(088386B0, link_course_pick) {
    uint32_t a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = b; c->r[7] = a;
    ARG(1) = LINK;
    uint32_t used = (uint32_t)S8(LINK + 858);
    c->r[9] = used;
    *W(c->r[29] + 16) = ra;
    uint32_t n = 0;
    while (n < 6 && (used & (1u << n)) != 0) n++;
    c->r[10] = n;
    *B(LINK + 856) = (uint8_t)n;
    *B(LINK + 857) = 0;
    *W(LINK + 852) = *W(0x08A11DDCu + 4 * n);
    gaddr table = *W(LINK + 852);
    c->r[3] = table;
    gaddr stage = 0x08A12068u;
    c->r[2] = 0;
    c->r[11] = LINK + 4 * n;
    for (;;) {
        if ((int32_t)*W(stage + 8) < 0 && (int32_t)*W(stage + 12) < 0) {
            ARG(0) = used | (1u << (n & 31));
            *B(LINK + 858) = (uint8_t)ARG(0);
            ARG(0) = a; ARG(1) = b;
            c->r[31] = 0x08838768u; f_088386B0(c);
            c->r[31] = ra; c->r[29] += 0x20;
            return;
        }
        int found = 0;
        for (uint32_t k = 0; k < 3; k++) {
            int32_t next = (int32_t)*W(stage + 8 + 4 * k);
            if (next < 0) continue;
            if ((*W(LINK + 4 * n + 868) & (1u << ((uint32_t)next & 31))) == 0) continue;
            stage = *W(table + 4 * (uint32_t)next);
            c->r[2] = (uint32_t)next;
            found = 1;
            break;
        }
        if (!found) break;
    }
    *H(LINK + 892) = (uint16_t)c->r[2];
    c->r[8] = stage;
    *W(LINK + 860) = stage;
    *W(LINK + 864) = 0;
    for (uint32_t k = 0; k < 3; k++) {
        int32_t next = (int32_t)*W(stage + 8 + 4 * k);
        if (next >= 0) *W(LINK + 864) |= 1u << ((uint32_t)next & 31);
    }
    ARG(0) = 1;
    c->r[31] = ra; c->r[29] += 0x20;
}
