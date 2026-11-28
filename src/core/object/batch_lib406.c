/* Four routines that name a file with sprintf: the music changed, the music read ahead, and a
   module loaded from the disc. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND  0x08E69200u /* the sound's state: it begins with the music file's path; +164 the
                              track's volume, +165 how far its loading is, +166 its number,
                              +167 its stream's state, +172 the thread reading it ahead */
#define TRACKS 0x08AD7F28u /* 8 bytes a track: its name, and a byte for +164 */
#define PATH_FORMAT 0x08AD7F00u

void f_089E5F54(Cpu *c); void f_089F31AC(Cpu *c); void f_089E62F0(Cpu *c); void f_089E6244(Cpu *c);
void f_0887C724(Cpu *c); void f_089E600C(Cpu *c); void f_089F3470(Cpu *c);
void hle_sceKernelCreateThread(Cpu *c); void hle_sceKernelStartThread(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c); void hle_sceUmdGetDriveStat(Cpu *c); void hle_sceIoOpen(Cpu *c);
void hle_sceKernelLoadModuleByID(Cpu *c); void hle_sceIoClose(Cpu *c); void hle_sceKernelStartModule(Cpu *c);

/* The path of the track kept at +20 (or +36) of the frame is written at the sound's state. */
static gaddr track_path(Cpu *c, gaddr track_at, gaddr state_at, uint32_t back) {
    gaddr entry = TRACKS + ((uint32_t)*H(track_at) << 3);
    c->r[16] = entry;
    ARG(2) = *W(entry);
    ARG(0) = *W(state_at);
    ARG(1) = PATH_FORMAT;
    c->r[31] = back; f_089F31AC(c);
    return entry;
}

/* (track): the music changes to the track, unless it is the one playing: what plays is stopped,
   the track's path and volume set, and its stream started. A 0x20 frame keeping s0. */
SFA_FUNC(089E63C8, music_change) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    uint32_t track = ARG(0) & 0xFFFF;
    ARG(1) = track;
    ARG(0) = SOUND;
    c->r[6] = *B(SOUND + 166);
    *W(sp + 24) = s0; *W(sp + 28) = ra;
    if (track != c->r[6]) {
        *H(sp + 20) = (uint16_t)track;
        *W(sp + 16) = SOUND;
        c->r[31] = 0x089E63F8u; f_089E5F54(c);
        gaddr entry = track_path(c, sp + 20, sp + 16, 0x089E6420u);
        ARG(0) = *B(entry + 4);
        gaddr s = *W(sp + 16);
        ARG(1) = s;
        c->r[6] = *H(sp + 20);
        *B(s + 164) = (uint8_t)ARG(0);
        *B(s + 166) = (uint8_t)c->r[6];
        *B(s + 167) = 0;
        *B(s + 165) = 0;
        c->r[31] = 0x089E6440u; f_089E62F0(c);
        c->r[31] = 0x089E6448u; f_089E6244(c);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): the music for the game's number n: the track it stands for, then the change. A 0x20
   frame. */
SFA_FUNC(0887C6E4, music_play_by_number) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) &= 0xFFFF; c->r[31] = 0x0887C6F4u; f_0887C724(c);
    ARG(0) = c->r[2] & 0xFF; c->r[31] = 0x0887C6FCu; f_089E63C8(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (track): the track is read ahead while something else plays: for the one playing, +165 just
   becomes 2; else what was being read is given up, the track's path and volume set, and a
   thread (its routine at 0x089E5FB8, kept at +172) started to read the file. A 0x30 frame
   keeping s0. */
SFA_FUNC(089E6050, music_read_ahead) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t track = ARG(0) & 0xFFFF;
    ARG(1) = track;
    ARG(0) = SOUND;
    c->r[6] = *B(SOUND + 166);
    *W(sp + 40) = s0; *W(sp + 44) = ra;
    if (track == c->r[6]) {
        ARG(1) = 2;
        *B(SOUND + 165) = 2;
    } else {
        *H(sp + 36) = (uint16_t)track;
        *W(sp + 32) = SOUND;
        c->r[31] = 0x089E6094u; f_089E600C(c);
        c->r[31] = 0x089E609Cu; f_089E5F54(c);
        gaddr entry = track_path(c, sp + 36, sp + 32, 0x089E60C4u);
        gaddr s = *W(sp + 32);
        *B(s + 164) = *B(entry + 4);
        *B(s + 166) = (uint8_t)*H(sp + 36);
        *B(s + 165) = 1;
        *B(s + 167) = 0;
        ARG(0) = 0x08A0207Cu; ARG(1) = 0x089E5FB8u; ARG(2) = 0x11; ARG(3) = 0x10000;
        c->r[8] = 0; c->r[9] = 0;
        c->r[31] = 0x089E6108u; hle_sceKernelCreateThread(c);
        ARG(1) = *W(sp + 32);
        ARG(0) = c->r[2];
        *W(ARG(1) + 172) = c->r[2];
        ARG(1) = 0; ARG(2) = 0;
        c->r[31] = 0x089E6120u; hle_sceKernelStartThread(c);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (name, where): a module is loaded from the disc and started: its path is made from the name
   (the format at 0x08A03763), and once the disc is ready and the file opens (a millisecond
   between tries, and again from the start if the load fails) it is loaded with the options in
   the frame (+164: their size 0x14, `where` twice, and two bytes 0 and 1) and started. The word
   at 0x08A03788 is 1 while the file is open. Returns the module's number. A 0xE0 frame keeping
   s0..s6, with the path at +32 and the start's answer at +160. */
SFA_FUNC(088082D0, module_load_start) {
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0xE0;
    gaddr sp = c->r[29];
    *W(sp + 184) = sv[0]; *W(sp + 188) = sv[1]; *W(sp + 204) = sv[5];
    c->r[16] = ARG(1);
    c->r[17] = ARG(0);
    c->r[21] = sp + 0xA4;
    *W(sp + 192) = sv[2];
    c->r[18] = 0x14;
    *W(sp + 196) = sv[3]; *W(sp + 200) = sv[4]; *W(sp + 208) = sv[6]; *W(sp + 212) = ra;
    ARG(0) = sp + 0xA4; ARG(1) = 0; ARG(2) = 0x14;
    c->r[31] = 0x08808314u; f_089F3470(c);
    *W(sp + 164) = 0x14;
    *W(sp + 168) = c->r[16];
    *W(sp + 172) = c->r[16];
    c->r[20] = 1;
    *B(sp + 180) = 0;
    c->r[19] = sp + 0x20;
    *B(sp + 181) = 1;
    ARG(0) = sp + 0x20; ARG(2) = c->r[17]; ARG(1) = 0x08A03763u;
    c->r[31] = 0x08808344u; f_089F31AC(c);
    c->r[18] = 0x02000001u;
    c->r[17] = 0x08A00000u;
    for (;;) {
        ARG(0) = 0x3E8; c->r[31] = 0x08808358u; hle_sceKernelDelayThread(c);
        c->r[31] = 0x08808360u; hle_sceUmdGetDriveStat(c);
        if ((c->r[2] & 0x20) == 0) continue;
        ARG(0) = c->r[19]; ARG(1) = c->r[18]; ARG(2) = 0;
        c->r[31] = 0x08808378u; hle_sceIoOpen(c);
        c->r[22] = c->r[2];
        if ((int32_t)c->r[22] < 0) continue;
        *W(0x08A03788u) = 1;
        ARG(0) = c->r[22]; ARG(1) = 0; ARG(2) = c->r[21];
        c->r[31] = 0x08808398u; hle_sceKernelLoadModuleByID(c);
        c->r[16] = c->r[2];
        c->r[7] = sp + 0xA0;
        if ((int32_t)c->r[16] >= 0) break;
        ARG(0) = c->r[22]; c->r[31] = 0x088083ACu; hle_sceIoClose(c);
        *W(0x08A03788u) = 0;
    }
    ARG(0) = c->r[16]; ARG(1) = 0; ARG(2) = 0; c->r[8] = 0;
    c->r[31] = 0x088083C8u; hle_sceKernelStartModule(c);
    ARG(0) = c->r[22]; c->r[31] = 0x088083D0u; hle_sceIoClose(c);
    *W(0x08A03788u) = 0;
    c->r[2] = c->r[16];
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0xE0;
}
