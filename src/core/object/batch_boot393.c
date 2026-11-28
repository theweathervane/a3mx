/* The program's start and three routines it leans on: the module's entry that starts the main
   thread, the wait for the disc, the wait for a load at the start, and the part of the heap's
   growing that asks the system for its block. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D7EA0(Cpu *c); void f_088075F8(Cpu *c); void f_088054F0(Cpu *c);
void hle_sceKernelSetCompiledSdkVersion(Cpu *c); void hle_sceKernelSetCompilerVersion(Cpu *c);
void hle_sceKernelCreateThread(Cpu *c); void hle_sceKernelStartThread(Cpu *c);
void hle_sceUmdCheckMedium(Cpu *c); void hle_sceUmdWaitDriveStat(Cpu *c); void hle_sceUmdActivate(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c); void hle_sceKernelAllocPartitionMemory(Cpu *c);
void hle_sceKernelGetBlockHeadAddr(Cpu *c);

/* (argc, argp): the module's entry: the system is told the versions the program was built with,
   and the main thread (its name at 0x089FD028, its routine at 0x088040BC) is made with priority
   0x20, a stack of 0x40000 and the user attribute, and started with the arguments. The three
   settings a program can give for these are absent here (the places they would be read from are
   0), so the defaults stand. Returns 0. A 0x10 frame keeping s0, s1. */
SFA_FUNC(08804000, module_start) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = ARG(0);
    *W(sp + 8) = ra;
    *W(sp) = s0;
    c->r[16] = ARG(1);
    ARG(2) = 0x02050000u;
    ARG(0) = 0x02050010u;
    c->r[31] = 0x08804024u; hle_sceKernelSetCompiledSdkVersion(c);
    ARG(1) = 0x00030000u;
    ARG(0) = 0x00030303u;
    c->r[31] = 0x08804030u; hle_sceKernelSetCompilerVersion(c);
    ARG(0) = 0x089FD028u;
    c->r[3] = 0;
    c->r[2] = 0x08800000u;
    c->r[11] = 0; c->r[12] = 0;
    ARG(1) = 0x088040BCu;
    c->r[9] = 0;
    c->r[13] = 0;
    ARG(3) = 0x00040000u;
    ARG(2) = 0x20;
    c->r[8] = 0x80000000u;
    c->r[10] = 0x80000000u;
    c->r[31] = 0x08804094u; hle_sceKernelCreateThread(c);
    ARG(0) = c->r[2];
    ARG(1) = c->r[17];
    ARG(2) = c->r[16];
    c->r[31] = 0x088040A4u; hle_sceKernelStartThread(c);
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}

/* A part of the heap's growing (s0 the page its state is on, s1 the size wanted of the system,
   s2 how much the caller asks for; r2 the page of the block's name): the system is asked for a
   block of s1 bytes (kept at 0x08AD8FCC); when it gives one, the block's start becomes both the
   heap's next free place (0x08AD8FC0) and its start (s0's -28732), and its end the heap's end
   (0x08AD8FC8). Then the caller's piece is taken from the next free place: -1 when there is no
   heap or it does not reach. The end of a 0x10 frame that kept s0..s2. */
SFA_FUNC(08804660, tail_heap_block) {
    gaddr sp = c->r[29];
    ARG(0) = 2;
    ARG(1) = c->r[2] + 0xFFFFD070u;
    ARG(2) = 3;
    ARG(3) = c->r[17];
    c->r[8] = 0x1000;
    c->r[31] = 0x08804678u; hle_sceKernelAllocPartitionMemory(c);
    c->r[3] = 0x08AE0000u;
    ARG(0) = c->r[2];
    *W(0x08AD8FCCu) = c->r[2];
    if ((int32_t)c->r[2] > 0) {
        c->r[31] = 0x08804690u; hle_sceKernelGetBlockHeadAddr(c);
        c->r[7] = c->r[2] + c->r[17];
        c->r[6] = 0x08AE0000u; ARG(1) = 0x08AE0000u;
        *W(0x08AD8FC8u) = c->r[7];
        *W(0x08AD8FC0u) = c->r[2];
        *W(c->r[16] - 28732) = c->r[2];
    }
    uint32_t start = *W(c->r[16] - 28732);
    c->r[2] = 0xFFFFFFFFu;
    if (start != 0) {
        c->r[6] = 0x08AE0000u;
        uint32_t next = *W(0x08AD8FC0u);
        ARG(1) = next;
        uint32_t after = next + c->r[18];
        ARG(0) = after;
        c->r[8] = after < start;
        if (after >= start) {
            c->r[11] = 0x08AE0000u;
            c->r[10] = *W(0x08AD8FC8u);
            c->r[9] = c->r[10] < after;
            if (c->r[9] == 0) {
                c->r[2] = next;
                *W(0x08AD8FC0u) = after;
            }
        }
    }
    c->r[31] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[29] += 0x10;
}

/* The wait for the disc: until it can be made active (a millisecond between tries; when no disc
   is in, the drive is waited on first), and then until it is ready to read. A 0x20 frame
   keeping s0. */
SFA_FUNC(08807380, disc_wait_ready) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = 0x089FDFC4u;
    *W(c->r[29] + 20) = ra;
    for (;;) {
        c->r[31] = 0x0880739Cu; hle_sceUmdCheckMedium(c);
        ARG(0) = 1;
        if (c->r[2] == 0) {
            ARG(0) = 2; c->r[31] = 0x088073ACu; hle_sceUmdWaitDriveStat(c);
            ARG(0) = 1;
        }
        ARG(1) = c->r[16];
        c->r[31] = 0x088073B8u; hle_sceUmdActivate(c);
        if ((int32_t)c->r[2] >= 0) break;
        ARG(0) = 0x3E8; c->r[31] = 0x088073C8u; hle_sceKernelDelayThread(c);
    }
    ARG(0) = 0x20; c->r[31] = 0x088073D8u; hle_sceUmdWaitDriveStat(c);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The first load: with the routine at 0x088D7EA0 told 1 before and 0 after, the current record
   of 216 bytes (its number the byte at 0x08E76084, the records from 0x08E76088) has four of its
   fields cleared, three objects' +56 are set from the tables at 0x08A0562C, 0x08A056D4 and
   0x08A0577C, the file the table at 0x08A03140 names at +108 is asked for, and frames are run
   until the byte at 0x08E7606C says it is there; then three words from 0x08E75C48 are set to
   -1. A 0x20 frame keeping s0. */
SFA_FUNC(08813C04, first_load_wait) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    ARG(0) = 1; c->r[31] = 0x08813C18u; f_088D7EA0(c);
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 202) = 0;
    *B(rec + 203) = 0;
    *H(rec + 208) = 0;
    *B(rec + 204) = 0;
    ARG(0) = *W(0x08A03140u + 108);
    *W(0x08E75AD8u + 56) = *W(0x08A0562Cu + 56);
    *W(0x092D9D40u + 56) = *W(0x08A056D4u + 56);
    *W(0x092D9E00u + 56) = *W(0x08A0577Cu + 56);
    ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08813CACu; f_088075F8(c);
    c->r[16] = 0x08E76068u;
    while (*B(0x08E7606Cu) == 0) {
        ARG(0) = 1; c->r[31] = 0x08813CC8u; f_088054F0(c);
    }
    ARG(1) = 0x08E70000u;
    ARG(2) = 0x08E75C48u;
    *H(0x08E75C48u + 4) = 0xFFFF;
    *H(0x08E75C48u + 2) = 0xFFFF;
    *H(0x08E75C48u) = 0xFFFF;
    ARG(0) = 0; c->r[31] = 0x08813CF4u; f_088D7EA0(c);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
