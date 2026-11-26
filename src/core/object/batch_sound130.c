/* Small sound-cue callers, 8 - 11 instructions: an object's fixed cues from its side (+337), and menu
   cues queued plainly. */
#include "step.h"
#include "../guest_mem.h"

void f_0887C408(Cpu *c); void f_0887C4CC(Cpu *c);

/* (object, cue): the cue from the object's side (+337). */
static void obj_cue(Cpu *c, gaddr o, uint32_t cue, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = cue;
    c->r[31] = back; f_0887C408(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* A plain cue at volume 0, pan 0xC0. */
static void menu_cue(Cpu *c, uint32_t cue, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0; ARG(1) = cue; ARG(2) = 0xC0;
    c->r[31] = back; f_0887C4CC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* With the task's +10 clear, a plain cue; else nothing. */
static void menu_cue_unless(Cpu *c, uint32_t cue, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t busy = *B(ARG(0) + 10);
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0;
    if (busy == 0) { ARG(1) = cue; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

SFA_FUNC(088D2D14, obj_cue_30) { obj_cue(c, ARG(0), 0x30, 0x088D2D28u); }
SFA_FUNC(088D2D54, obj_cue_27) { obj_cue(c, ARG(0), 0x27, 0x088D2D68u); }
SFA_FUNC(088D2D74, obj_cue_06) { obj_cue(c, ARG(0), 0x06, 0x088D2D88u); }
SFA_FUNC(088D2DBC, obj_cue_22) { obj_cue(c, ARG(0), 0x22, 0x088D2DD0u); }

/* (object): cue 0x33 + its +386. */
SFA_FUNC(0887C65C, obj_cue_386) { obj_cue(c, ARG(0), (uint32_t)(S8(ARG(0) + 386) + 0x33), 0x0887C674u); }

/* (object, n): cue 0x200 | n's low byte (group 2: from the side's bank). */
SFA_FUNC(088D2E04, obj_cue_bank) { obj_cue(c, ARG(0), (ARG(1) & 0xFF) | 0x200, 0x088D2E1Cu); }

SFA_FUNC(088D55F8, menu_cue_38) { menu_cue(c, 0x38, 0x088D5610u); }
SFA_FUNC(088D561C, menu_cue_39) { menu_cue(c, 0x39, 0x088D5634u); }
SFA_FUNC(088D5640, menu_cue_2f) { menu_cue(c, 0x2F, 0x088D5658u); }

/* (task): cue 0x39 / 0x38 unless the task's +10 is set. */
SFA_FUNC(0889D01C, menu_cue_39_unless) { menu_cue_unless(c, 0x39, 0x0889D03Cu); }
SFA_FUNC(0889D048, menu_cue_38_unless) { menu_cue_unless(c, 0x38, 0x0889D068u); }
