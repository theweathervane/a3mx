/* Eight more of the link's pieces: two more of its menu's rows of steps, the kept names looked
   after once a frame, packets sent, and whether everyone in the room agrees. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E3ED8(Cpu *c); void f_088E3FD0(Cpu *c); void f_088D6950(Cpu *c); void f_088E3F44(Cpu *c);
void f_088DD164(Cpu *c); void f_088DF778(Cpu *c); void f_088D68C8(Cpu *c); void f_088DD02C(Cpu *c);
void f_088DD114(Cpu *c); void f_088D5680(Cpu *c); void f_088DCB00(Cpu *c); void f_088DF208(Cpu *c);
void f_088E1444(Cpu *c); void f_088DD124(Cpu *c); void f_088DF754(Cpu *c); void f_088DC42C(Cpu *c);
void f_0880A6CC(Cpu *c); void f_089E4F28(Cpu *c); void f_088DD8AC(Cpu *c); void f_088DDA8C(Cpu *c);
void f_088E426C(Cpu *c); void f_088E1468(Cpu *c); void f_088E187C(Cpu *c); void f_088DD370(Cpu *c);
void f_088DD380(Cpu *c); void f_088D56C4(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void step_on(gaddr p) { *H(p) = (uint16_t)(*H(p) + 1); }
static void leave_eb14(gaddr p) {
    *W(p + 20) = 0x08A3EB14u;
    *H(p + 24) = 0; *H(p + 2) = 0; *H(p) = 0;
    *B(p + 47) = 0;
}
/* The row is over: +24 counts on and the steps start again. */
static void row_over(gaddr p) {
    *H(p + 24) = (uint16_t)(*H(p + 24) + 1);
    *H(p + 2) = 0;
    *H(p) = 0;
    *B(p + 47) = 0;
}

/* (menu): the room's players are shown for a while: 0 starts (0x088E3ED8 with 0xFF, 0x1E0 frames
   at +4) and goes straight on to 1, which waits for 0x088E3FD0; 2 shows the list while the frames
   run out; 3 waits for 0x088E3F44; 4 keeps 0x088DD164's answer at +12 and ends the row. Past 4
   0x088DF778 runs and the menu leaves. The link's own line is drawn every frame. */
SFA_FUNC(088D732C, room_shown_steps) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = p;
    uint32_t step = *H(p);
    *W(c->r[29] + 20) = ra;
    if (!(step < 5)) {
        call0(c, f_088DF778, 0x088D7434u);
        leave_eb14(p);
    } else if (step == 0 || step == 1) {
        if (step == 0) {
            call1(c, 0xFF, f_088E3ED8, 0x088D7378u);
            *H(p + 4) = 0x1E0;
            step_on(p);
        }
        call0(c, f_088E3FD0, 0x088D7394u);
        if (c->r[2] != 0) step_on(p);
    } else if (step == 2) {
        call2(c, p, 0, f_088D6950, 0x088D73B8u);
        int32_t t = S16(p + 4) - 1;
        *H(p + 4) = (uint16_t)t;
        if ((int16_t)t == 0) step_on(p);
    } else if (step == 3) {
        call0(c, f_088E3F44, 0x088D73ECu);
        if (c->r[2] != 0) step_on(p);
    } else {
        call1(c, 1, f_088DD164, 0x088D740Cu);
        *W(p + 12) = c->r[2];
        row_over(p);
    }
    call1(c, p, f_088D68C8, 0x088D7458u);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (menu): the room waits for its players: 0 starts (0x168 frames at +4, 0x088DD02C, 0x088E3ED8
   with 2) and goes on to 1, which with +44 set wants as many players as +54 says (else
   0x088D5680 with 0x81001003); 2 calls off a stranger's request (0x088DCB00 naming anyone but
   the one at +28), and otherwise waits: with +32 clear not at all, with the partner's +144 at 4
   0x168 frames, else until the frames run out and 0xB4 more; 3 lets those run out; 4 forgets
   the partner and closes (0x088E1444, 0x088DF754 with 0x088DD124's answer, 0x088DC42C); 5 ends
   the row. Past 5 the menu leaves. The choices' line of text and the link's own line are drawn
   every frame. A 0x40 frame keeping s0..s3. */
SFA_FUNC(088DA414, room_wait_steps) {
    gaddr p = ARG(0);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = p;
    *W(sp + 36) = sv[1];
    gaddr partner = *W(p + 28);
    c->r[17] = partner;
    uint32_t step = *H(p);
    *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4];
    c->r[19] = 4;
    if (!(step < 6)) leave_eb14(p);
    else if (step == 0 || step == 1) {
        if (step == 0) {
            *H(p) = (uint16_t)(step + 1);
            *H(p + 4) = 0x168;
            call2(c, step + 1, 0x168, f_088DD02C, 0x088DA484u);
            call1(c, 2, f_088E3ED8, 0x088DA48Cu);
        }
        if (*B(p + 44) == 0) step_on(p);
        else {
            call0(c, f_088DD114, 0x088DA4B8u);
            int32_t n = (int32_t)c->r[2];
            if (n < 1 || (uint32_t)n != *B(p + 54)) call2(c, p, 0x81001003u, f_088D5680, 0x088DA4E4u);
            else step_on(p);
        }
    } else if (step == 5) row_over(p);
    else {
        int go = 1;             /* on to the frames running out */
        int32_t left = 0;
        if (step == 2) {
            call0(c, f_088DCB00, 0x088DA504u);
            gaddr who = c->r[2];
            c->r[18] = who;
            if (who != 0 && who != partner) {
                call1(c, who + 4, f_088DF208, 0x088DA520u);
                *B(who + 154) = 0;
                go = 0;
            } else if (*B(p + 32) == 0) {
                uint32_t next = (uint32_t)*H(p) + 1;
                *H(p + 4) = 0;
                left = S16(p + 4);
                *H(p) = (uint16_t)next;
            } else if (*W(partner + 144) == 4) {
                uint32_t next = (uint32_t)*H(p) + 1;
                *H(p + 4) = 0x168;
                left = S16(p + 4);
                *H(p) = (uint16_t)next;
            } else {
                int32_t t = S16(p + 4) - 1;
                *H(p + 4) = (uint16_t)t;
                if ((int16_t)t != 0) go = 0;
                else {
                    uint32_t next = (uint32_t)*H(p) + 1;
                    *H(p + 4) = 0xB4;
                    left = S16(p + 4);
                    *H(p) = (uint16_t)next;
                }
            }
        } else if (step == 3) left = S16(p + 4);
        if (go && step != 4) {
            if (left > 0) { *H(p + 4) = (uint16_t)(left - 1); go = 0; }
            else step_on(p);
        }
        if (go) {
            *W(p + 28) = 0;
            call0(c, f_088E1444, 0x088DA5BCu);
            call0(c, f_088DD124, 0x088DA5C4u);
            call1(c, c->r[2], f_088DF754, 0x088DA5CCu);
            call0(c, f_088DC42C, 0x088DA5D4u);
            step_on(p);
            row_over(p);
        }
    }
    ARG(3) = 4; c->r[8] = 9;
    call3(c, *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2)), 2, 0x30, f_0880A6CC, 0x088DA658u);
    call1(c, p, f_088D68C8, 0x088DA660u);
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* The kept names, once a frame: a record still counting (+12) counts down; one no longer wanted
   (+13 clear) has the strip at 0x08B02EC0 drawn with the text at 0x08A012DC and is taken out
   (0x088DD8AC); one marked (+14) is drawn again. A 0x30 frame keeping s0..s3. */
SFA_FUNC(088DDB78, kept_names_frame) {
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    gaddr cur = *W(0x08B02EA8u);
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4];
    gaddr prev = 0;
    c->r[18] = 0;
    if (cur != 0) { c->r[17] = 0x08B02EC0u; c->r[16] = 0x08A012DCu; }
    while (cur != 0) {
        c->r[19] = cur;
        int32_t t = S8(cur + 12);
        if (t > 0) {
            *B(cur + 12) = (uint8_t)(t - 1);
            prev = cur;
            cur = *W(prev);
        } else if (S8(cur + 13) == 0) {
            call2(c, 0x08B02EC0u, 0x08A012DCu, f_089E4F28, 0x088DDBE0u);
            call2(c, cur, prev, f_088DD8AC, 0x088DDBECu);
            cur = c->r[2];
        } else {
            prev = cur;
            c->r[18] = prev;
            if (S8(cur + 14) != 0) call1(c, cur, f_088DDA8C, 0x088DDC08u);
            cur = *W(prev);
        }
        c->r[18] = prev;
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (packet): a packet is sent: its check (0x088E426C over the size its kind names) goes to the
   header's +2, and the record is handed over. */
SFA_FUNC(088E43B0, packet_send) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    gaddr head = x - 4;
    c->r[16] = head;
    uint32_t size = *W(0x08A3EC50u + (((uint32_t)*B(head + 1) & 0xF) << 2));
    *W(c->r[29] + 20) = ra;
    ARG(1) = size;
    call0(c, f_088E426C, 0x088E43E0u);
    *H(head + 2) = (uint16_t)c->r[2];
    call1(c, head - 8, f_088E1468, 0x088E43ECu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (packet, n): a packet waits to be sent with n (0xF0 when n is not above 0), its check filled
   in; without a packet the queue gets (0, 0). A frame keeping s0, s1. */
SFA_FUNC(088E43FC, packet_queue) {
    gaddr x = ARG(0);
    int32_t n = (int32_t)ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = (uint32_t)n;
    if (x == 0) call2(c, 0, 0, f_088E187C, 0x088E4478u);
    else {
        gaddr head = x - 4;
        c->r[17] = head;
        ARG(1) = *W(0x08A3EC50u + (((uint32_t)*B(head + 1) & 0xF) << 2));
        call0(c, f_088E426C, 0x088E4438u);
        *H(head + 2) = (uint16_t)c->r[2];
        if (n < 1) n = 0xF0;
        c->r[16] = (uint32_t)n;
        c->r[17] = head - 8;
        call2(c, head - 8, (uint32_t)n, f_088E187C, 0x088E4464u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* 1 when every player of the room (0x088DD380, from the last down) has the byte `off` equal to
   `want`; 0 when one differs or is missing. */
static uint32_t all_agree(Cpu *c, uint32_t want, uint32_t off, uint32_t back_count, uint32_t back_each) {
    call0(c, f_088DD114, back_count);
    int32_t i = (int32_t)c->r[2];
    c->r[16] = (uint32_t)i;
    while (i >= 0) {
        call1(c, (uint32_t)i, f_088DD380, back_each);
        gaddr who = c->r[2];
        if (who == 0) return 0;
        i--;
        c->r[16] = (uint32_t)i;
        if (*B(who + off) != want) return 0;
    }
    return 1;
}

/* 1 when every player's +3 is the same as the one at 0x09523903. A frame keeping s0, s1. */
SFA_FUNC(088E456C, all_agree_3) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    call0(c, f_088DD370, 0x088E4584u);
    uint32_t want = *B(c->r[2] + 3);
    c->r[17] = want;
    c->r[2] = all_agree(c, want, 3, 0x088E458Cu, 0x088E45A0u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (v): 1 when every player's +2 is v, or the one at 0x09523902 when v is 0. A frame keeping s0,
   s1. */
SFA_FUNC(088E45F0, all_agree_2) {
    uint32_t want = ARG(0) & 0xFF;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = want;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    if (want == 0) {
        call0(c, f_088DD370, 0x088E4614u);
        want = *B(c->r[2] + 2);
        c->r[17] = want;
    }
    c->r[2] = all_agree(c, want, 2, 0x088E4620u, 0x088E4634u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (menu): the menu's row by its first word: 0x14 and 0x64 leave for the screen at 0x08A3EB14;
   the others run the step the table 0x08A3F0F0 has for them, when it has one. */
SFA_FUNC(088E696C, menu_row_run) {
    gaddr p = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t row = *H(p);
    *W(c->r[29] + 16) = ra;
    if (row == 0x64 || row == 0x14) call0(c, f_088D56C4, 0x088E6994u);
    else {
        uint32_t fn = *W(0x08A3F0F0u + (row << 2));
        c->r[5] = fn; c->r[6] = 0x08A3F0F0u;
        if (fn != 0) { c->r[31] = 0x088E69C0u; rt_call_indirect(c, fn); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
