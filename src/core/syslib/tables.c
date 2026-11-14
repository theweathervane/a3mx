/* Small system-side tables between the 2D library and the save data (0x089E1D3C - 0x089E31E0).
 *
 * Three tables of records whose word 0 is 1 while in use, handed out by address:
 *   TABLE_A  8 records of 20 bytes, cleared when taken
 *   TABLE_B 16 records of 20 bytes: +4 a value from -128..127 scaled to 0 - 0x1000 round 0x800,
 *           +8 an s8, +12 0x1000 when taken
 *   TABLE_C  8 records of 24 bytes: +4 a value from -128..127 plus 127, kept to 127 at most, +8 the
 *           same halved, +16 -1 when taken
 * Taking a record when all are in use answers 0x07000101 with no record.
 */
#include "../decomp.h"

#define TABLE_A 0x08ACCE10u
#define TABLE_B 0x08AD3AF8u
#define TABLE_C 0x08AD3C38u
#define ALL_IN_USE 0x07000101u

#define SAVE_NAMES 0x08AD3D8Cu   /* four 13-byte names: STFALPH3.SYS, .WTR, .NET, .DWN */

static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* The first record not in use, or 0. */
static gaddr free_record(gaddr table, int count, uint32_t size) {
    for (int i = 0; i < count; i++) if (*W(table + (uint32_t)i * size) == 0) return table + (uint32_t)i * size;
    return 0;
}

/* The record at this address, or 0 when it is none of the table's. */
static gaddr find_record(gaddr table, int count, uint32_t size, gaddr r) {
    for (int i = 0; i < count; i++) if (table + (uint32_t)i * size == r) return r;
    return 0;
}

/* (out): a record taken, its address in *out (0 when there is none). */
static void take(Cpu *c, gaddr table, int count, uint32_t size, const uint32_t *init, int words) {
    gaddr out = ARG(0), r = free_record(table, count, size);
    if (r == 0) { *W(out) = 0; RETURN(ALL_IN_USE); return; }
    *W(r) = 1;
    for (int i = 0; i < words; i++) W(r + 4)[i] = init[i];
    *W(out) = r;
    RETURN(0);
}

SFA_FUNC(089E1D3C, table_a_take) { static const uint32_t v[] = { 0, 0, 0, 0 };                take(c, TABLE_A, 8, 20, v, 4); }
SFA_FUNC(089E26D0, table_b_take) { static const uint32_t v[] = { 0x800, 0, 0x1000 };          take(c, TABLE_B, 16, 20, v, 3); }
SFA_FUNC(089E272C, table_c_take) { static const uint32_t v[] = { 0x40, 0x40, 0, 0xFFFFFFFFu }; take(c, TABLE_C, 8, 24, v, 4); }

/* (record, v): v (its low byte, signed) scaled to 0 - 0x1000 round 0x800 into +4. */
SFA_FUNC(089E2340, table_b_set_4) {
    gaddr r = find_record(TABLE_B, 16, 20, ARG(0));
    int32_t v = (int8_t)ARG(1);
    if (r != 0) {
        int32_t scaled = (v << 11) / 127;
        c->lo = (uint32_t)scaled; c->hi = (uint32_t)((v << 11) % 127);
        *W(r + 4) = (uint32_t)(scaled + 0x800);
    }
    RETURN(0);
}

/* (record, v): v (its low byte, signed) into +8. */
SFA_FUNC(089E23A0, table_b_set_8) {
    gaddr r = find_record(TABLE_B, 16, 20, ARG(0));
    if (r != 0) *W(r + 8) = (uint32_t)(int32_t)(int8_t)ARG(1);
    RETURN(0);
}

/* (record, v): v (its low byte, signed) plus 127 into +4, kept to 127 at most. */
SFA_FUNC(089E25E8, table_c_set_4) {
    gaddr r = find_record(TABLE_C, 8, 24, ARG(0));
    if (r != 0) {
        int32_t v = (int8_t)ARG(1) + 0x7F;
        *W(r + 4) = (uint32_t)(v < 128 ? v : 0x7F);
    }
    RETURN(0);
}

/* (record, v): v (its low byte, signed) plus 127, halved, into +8. */
SFA_FUNC(089E2644, table_c_set_8) {
    gaddr r = find_record(TABLE_C, 8, 24, ARG(0));
    if (r != 0) *W(r + 8) = (uint32_t)(((int8_t)ARG(1) + 0x7F) / 2);
    RETURN(0);
}

/* (record): looks the record up in TABLE_C and does nothing with it. */
SFA_FUNC(089E26A0, table_c_nothing) { RETURN(0); }

/* ---- the save data ---- */

void f_089F3470(Cpu *c);   /* memset */
void f_089F34A4(Cpu *c);   /* strcmp */
void f_089F34D8(Cpu *c);   /* strcpy */

/* (name): which of the four save files a name is (0 - 3); 0 too for none of them. */
SFA_FUNC(089E3158, save_file_number) {
    gaddr name = ARG(0);
    int found = 0;
    c->r[29] -= 0x20;
    for (int i = 0; i < 4; i++)
        if (CALL2(f_089F34A4, name, SAVE_NAMES + (uint32_t)i * 13u) == 0) { found = i; break; }
    c->r[29] += 0x20;
    RETURN(found);
}

/* (params): the 0x600-byte parameters of a system dialogue cleared, then the common part: its size,
   language 1, button swap 1, and the graphics, access, font and sound threads' priorities 0x11, 0x13,
   0x12, 0x10. */
SFA_FUNC(089E31E0, utility_params_init) {
    gaddr p = ARG(0);
    c->r[29] -= 0x20;
    CALL3(f_089F3470, p, 0, 0x600);
    c->r[29] += 0x20;
    static const uint32_t common[] = { 0x600, 1, 1, 0x11, 0x13, 0x12, 0x10 };
    for (int i = 0; i < 7; i++) W(p)[i] = common[i];
}

#define GAME_NAME   0x08A01DC8u   /* "ULUS10062" */
#define SAVE_DIRS   0x08AD3D3Cu   /* four 20-byte names: "0000" - "0003" */

/* (params, n): save data parameters for save file n: the common part (above), then the game's name
   (+0x3C), the save's name (+0x4C) and the file's name (+0x64). */
SFA_FUNC(089E3248, save_params_init) {
    gaddr p = ARG(0);
    uint32_t n = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = n;
    c->r[4] = p; f_089E31E0(c);
    CALL2(f_089F34D8, p + 0x3C, GAME_NAME);
    CALL2(f_089F34D8, p + 0x4C, SAVE_DIRS + n * 20u);
    CALL2(f_089F34D8, p + 0x64, SAVE_NAMES + n * 13u);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
