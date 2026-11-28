/* A save's start from the save screen, and more of the library: the sound thread stopped and
   started, a module's state, and a callback entered for a record. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND 0x08E69200u /* the sound driver's state */

void f_089E0DC4(Cpu *c); void f_089E30F0(Cpu *c); void f_089E3AB0(Cpu *c); void f_08816FBC(Cpu *c);
void f_089E39B8(Cpu *c); void f_089F0040(Cpu *c); void f_089EE594(Cpu *c); void f_089E59D4(Cpu *c);
void f_089E5B30(Cpu *c); void f_089E5B4C(Cpu *c); void f_089E600C(Cpu *c); void f_089E5F54(Cpu *c);
void f_089E62F0(Cpu *c);
void hle_sceKernelWaitThreadEnd(Cpu *c); void hle_sceKernelDeleteThread(Cpu *c);
void hle_sceKernelCreateThread(Cpu *c); void hle_sceKernelStartThread(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (n, slot): a save from the save screen. The 0x12000 bytes at 0x08AEF4D8 are cleared and
   filled: for the first kind the mark 0x3399 at +640 and six blocks of the game's settings and
   records, for the second the mark and the 0x174C bytes at 0x09517E38. Then 0x089E30F0 with
   0x08AEF4CC, the byte at 0x0951F94B set, and the save itself (the n-th entry of 0x08A07768 and
   its size from 0x08A077B8); 0x08816FBC (1, slot) follows and the save's answer comes back. A
   0x40 frame keeping s0..s3. */
SFA_FUNC(088168E0, save_screen_save) {
    uint32_t n = ARG(0), slot = ARG(1);
    const gaddr data = 0x08AEF4D8u, head = 0x08AEF4CCu;
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 48) = sv[4];
    c->r[17] = n; c->r[16] = slot; c->r[18] = data; c->r[19] = head;
    *B(0x09523C60u + 6) = (uint8_t)slot;
    *B(0x09523C60u + 8) = 1;
    *B(0x09523C60u + 12) = (uint8_t)n;
    for (uint32_t i = 0; i < 18432; i++) *W(data + 4 * i) = 0;
    if ((int32_t)n == 0) {
        *W(data + 640) = 0x3399;
        call3(c, data + 0x284, 0x08A058A0u, 0x208, f_089E0DC4, 0x0881697Cu);
        call3(c, data + 0x48C, 0x08A05AA8u, 0x17C0, f_089E0DC4, 0x08816990u);
        call3(c, data + 0x1C4C, 0x08A07268u, 0x3C0, f_089E0DC4, 0x088169A4u);
        call3(c, data + 0x200C, 0x08A07628u, 0x140, f_089E0DC4, 0x088169B8u);
        call3(c, data + 0x214C, 0x09519588u, 0x13F8, f_089E0DC4, 0x088169CCu);
        call3(c, data + 0x3544, 0x08A05894u, 4, f_089E0DC4, 0x088169E0u);
    } else if ((int32_t)n == 1) {
        *W(data + 640) = 0x3399;
        call3(c, data + 0x284, 0x09517E38u, 0x174C, f_089E0DC4, 0x088169FCu);
    }
    call1(c, head, f_089E30F0, 0x08816A04u);
    *B(0x0951F94Bu) = 1;
    ARG(3) = *W(0x08A077B8u + (n << 2));
    c->r[8] = head; c->r[9] = 0x80000000u;
    call3(c, slot, 0x08A07768u + (n << 4), data, f_089E3AB0, 0x08816A48u);
    uint32_t answer = c->r[2];
    c->r[17] = answer;
    call2(c, 1, slot, f_08816FBC, 0x08816A58u);
    c->r[2] = answer;
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* (.., i): the room the i-th kind of save needs (its size from 0x08A077B8). */
SFA_FUNC(08816B90, save_room_by_kind) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t size = *W(0x08A077B8u + (ARG(1) << 2));
    ARG(1) = 0x08A077B8u;
    *W(c->r[29] + 16) = ra;
    call1(c, size, f_089E39B8, 0x08816BB0u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (i): the first byte of the i-th of the 0x81 records listed at 0x08E74308, or 0x80540007 when
   there is none. */
SFA_FUNC(089F0040, record_state) {
    uint32_t i = ARG(0);
    c->r[2] = 0x80540007u;
    if (i < 0x81) {
        gaddr p = *W(0x08E74308u + (i << 2));
        c->r[3] = p;
        if (p != 0) c->r[2] = *B(p);
    }
}

/* (i): the same, or 0x80450001 while the word at 0x08AD80D0 is -1. A 0x10 frame. */
SFA_FUNC(089EE594, record_state_checked) {
    uint32_t ra = c->r[31];
    uint32_t ready = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    if (ready == 0xFFFFFFFFu) c->r[2] = 0x80450001u;
    else call0(c, f_089F0040, 0x089EE5BCu);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (entry, a, b, priority): one of the eight entries at 0x08AD3C38 takes the record (a, b) of
   0x08D52380 (a below 6; 0xA414 bytes each, 0x148 for b) when that has something (+20): unless
   the entry's own is still busy (state 1) under a priority that is not lower, the record's +24
   is entered in the second callback table with the entry's +16, +4 and +8, and the entry keeps
   it and the priority. Returns 0. A frame keeping s0..s2. */
SFA_FUNC(089E2480, entry_takes_record) {
    gaddr entry = ARG(0);
    int32_t a = (int8_t)ARG(1), b = (int8_t)ARG(2), pr = (int8_t)ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; *W(sp + 16) = s0; *W(sp + 24) = s2;
    c->r[17] = (uint32_t)pr; c->r[18] = (uint32_t)b;
    *W(sp + 28) = ra;
    gaddr e = 0x08AD3C38u;
    uint32_t idx = 0;
    while (idx < 8 && e != entry) { idx++; e += 0x18; }
    c->r[16] = e;
    if (idx != 8 && a < 6) {
        int64_t m = (int64_t)a * 0xA414;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        gaddr rec = c->lo + (uint32_t)b * 0x148 + 0x08D52380u;
        c->r[18] = rec;
        if (*W(rec + 20) != 0) {
            call1(c, *W(e + 16), f_089EE594, 0x089E252Cu);
            int take = 1;
            if (c->r[2] == 1 && pr != 0) {
                int32_t held = (int32_t)*W(e + 20);
                if (held == 0 || pr < held) take = 0;
            }
            if (take) {
                ARG(3) = *W(e + 8);
                call3(c, *W(rec + 24), *W(e + 16), *W(e + 4), f_089E59D4, 0x089E2568u);
                *W(e + 20) = (uint32_t)pr;
                *W(e + 16) = *W(rec + 24);
            }
        }
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The sound thread is asked to stop (its flag's bit 8) and waited for when there is one
   (SOUND + 168), with the byte at 0x08AD80C2 set meanwhile; then all the flag's bits are
   cleared. A frame keeping s0, s1. */
SFA_FUNC(089E5F54, sound_thread_stop) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = SOUND;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    call2(c, *W(SOUND + 148), 8, f_089E5B30, 0x089E5F78u);
    uint32_t thread = *W(SOUND + 168);
    c->r[16] = 0x08AE0000u;
    if ((int32_t)thread > 0) {
        *B(0x08AD80C2u) = 1;
        call2(c, thread, 0, hle_sceKernelWaitThreadEnd, 0x089E5F94u);
    }
    uint32_t flag = *W(SOUND + 148);
    *B(0x08AD80C2u) = 0;
    call2(c, flag, 0xFFFFFFFFu, f_089E5B4C, 0x089E5FA4u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The loader's thread (SOUND + 172), when there is one, is waited for and deleted. */
SFA_FUNC(089E600C, loader_thread_end) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = SOUND;
    uint32_t thread = *W(SOUND + 172);
    *W(c->r[29] + 20) = ra;
    if (thread != 0) {
        call2(c, thread, 0, hle_sceKernelWaitThreadEnd, 0x089E6034u);
        call1(c, *W(SOUND + 172), hle_sceKernelDeleteThread, 0x089E603Cu);
        *W(SOUND + 172) = 0;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* Waits, a millisecond at a time, until the byte at SOUND + 167 is set. */
SFA_FUNC(089E6244, sound_wait_167) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = SOUND;
    *W(c->r[29] + 20) = ra;
    while (*B(SOUND + 167) == 0) call1(c, 0x3E8, hle_sceKernelDelayThread, 0x089E626Cu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The sound thread starts: its state from SOUND + 132 is set up (0xC8000, the flag from
   SOUND + 180, the tempo 0x8000), and the thread named at 0x08A02090 is created at 0x089E6288
   (priority 0x11, 0x10000 of stack) and started. Returns 0, or -1 when either fails. A 0x30
   frame keeping s0. */
SFA_FUNC(089E62F0, sound_thread_start) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0;
    c->r[16] = SOUND;
    *W(SOUND + 132) = 0xC8000;
    *W(SOUND + 136) = 0;
    *W(SOUND + 140) = 0;
    uint32_t flag = *W(SOUND + 180);
    *W(SOUND + 144) = 1;
    *W(SOUND + 148) = flag;
    *B(SOUND + 152) = 1;
    *W(SOUND + 156) = 0x8000;
    *W(SOUND + 160) = 0;
    ARG(2) = 0x11; c->r[8] = 0; c->r[9] = 0;
    *W(sp + 36) = ra;
    ARG(3) = 0x10000;
    call2(c, 0x08A02090u, 0x089E6288u, hle_sceKernelCreateThread, 0x089E6364u);
    uint32_t thread = c->r[2];
    *W(SOUND + 168) = thread;
    if ((int32_t)thread < 0) c->r[2] = 0xFFFFFFFFu;
    else {
        call3(c, thread, 0, 0, hle_sceKernelStartThread, 0x089E637Cu);
        c->r[2] = c->r[2] == 0 ? 0 : 0xFFFFFFFFu;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* The sound thread from the start: the loader's thread ended, the sound thread stopped and
   started again. A 0x10 frame. */
SFA_FUNC(089E6458, sound_thread_restart) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    call0(c, f_089E600C, 0x089E6468u);
    call0(c, f_089E5F54, 0x089E6470u);
    call0(c, f_089E62F0, 0x089E6478u);
    c->r[31] = ra; c->r[29] += 0x10;
}
