/* The loader thread: archives and files loaded in the background, a step each frame.
 *
 * queue_load (file.c) puts a name in the ring of 16 requests. The loader thread (0x0880775C) wakes
 * once a frame and runs one step of the first request that is waiting: each request is a small state
 * machine (LoadRequest.status), its states handled by the functions in the table at 0x08A03798:
 *
 *   0 OPEN          open the file (retried every frame until it opens)
 *   7 READ_HEADER   start reading the 2 KB archive header (a plain file gets a one-entry header made
 *                   up for it); pick the fighter heap layout
 *   1 HEADER_DONE   wait for the header; take the entry count and data size
 *   2 ENTRY_START   take the next entry: choose its destination (as pac_load does) and start reading
 *                   its first 32 KB
 *   3 CHUNK_DONE    wait for the chunk; copy it to the destination; start the next chunk, or finish
 *                   the entry (textures to the GPU list, fighter heap moved on) and go to the next
 *   4 SOUND         hand a finished sound / voice bank to the sound library, which calls back
 *                   (0x08807A38) when it has taken it; 5 waits for that
 *   6 FINISH        close the file, free what this archive replaced, release the request
 *   8 RETRY_OPEN    after a failed read: reopen, seek back to the chunk, read it again
 *   9 RETRY_READ    the drive was busy: start the read again
 *
 * The return value of a step goes to 0x08E7606F (the loader's "busy" byte).
 */
#include "../decomp.h"
#include "../psp_os.h"
#include "loader.h"

#define CURRENT_SLOT    0x08E87060u   /* u8: the request being worked on */
#define LOADER_STATE    0x08E76068u   /* +2 u16, +4 u8 all idle, +7 u8 the last step's result */
#define ASYNC_STAGING   0x08E76064u   /* the texture staging buffer the loader thread uses */
#define SOUND_HANDLE    0x08E76030u   /* the sound library handle for the bank being handed over */
#define LOAD_IDLE       0x08E75B98u   /* u8, cleared when a load starts or ends */
#define STATE_TABLE     0x08A03798u   /* the step functions, by state */
#define EXPECTED_SIZE   0x08A03790u   /* s64: a read must return at least this many bytes */
#define SOUND_CALLBACK  0x08807A38u   /* the sound library calls this when it has taken a bank */

enum { ST_OPEN = 0, ST_HEADER_DONE = 1, ST_ENTRY_START = 2, ST_CHUNK_DONE = 3, ST_SOUND = 4,
       ST_SOUND_WAIT = 5, ST_FINISH = 6, ST_READ_HEADER = 7, ST_RETRY_OPEN = 8, ST_RETRY_READ = 9 };

/* fighter heap layouts: the bytes already in use when a load starts, by LoadRequest.fighter_heap */
#define FIGHTER_HEAP_1  0x00170C00u
#define FIGHTER_HEAP_2  0x002E1800u
/* second per-fighter tables, by LoadRequest.fighter_heap */
#define FIGHTER_A_TABLE2 0x092D9EC0u
#define FIGHTER_B_TABLE2 0x092D9ED0u
#define FIGHTER_C_TABLE2 0x092D9EE0u

void f_088054F0(Cpu *c);   /* wait for the next frame (task/task.c) */
void f_089E04EC(Cpu *c);   /* allocate, aligned and zeroed */
void f_089E0540(Cpu *c);   /* free */
void f_089E055C(Cpu *c);   /* texture index -> VRAM address */
void f_089E0CF4(Cpu *c);   /* texture page: start */
void f_089E0DC4(Cpu *c);   /* copy */
void f_089E0DE0(Cpu *c);   /* texture list: add */
void f_089E0E70(Cpu *c);   /* texture list: upload slot */
void f_089E1D3C(Cpu *c);   /* sound library: open */
void f_089E1D94(Cpu *c);
void f_089E1D9C(Cpu *c);   /* sound library: give data (with a callback) */
void f_089E1E40(Cpu *c);   /* sound library: close */
void f_089E2AA0(Cpu *c);   /* sound library: buffer for a sequence */
void f_089E2B60(Cpu *c);   /* sound library: register sample bank */
void f_089E2E28(Cpu *c);   /* sound library: buffer for a bank */
void f_089E64BC(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline uint8_t *U8(gaddr a) { return guest_ptr(a); }
static inline int32_t s8(uint32_t v) { return (int8_t)v; }
static inline gaddr request_name(void) { return REQUEST_NAMES + *U8(CURRENT_SLOT) * 36u + 4; }

/* A finished background read: did it bring at least the expected byte count? (64-bit, signed) */
static int read_short(gaddr result) {
    int64_t got = (int64_t)((uint64_t)W(result)[1] << 32 | W(result)[0]);
    int64_t want = (int64_t)((uint64_t)W(EXPECTED_SIZE)[1] << 32 | W(EXPECTED_SIZE)[0]);
    return got < want;
}

/* Starts reading the next chunk; the state follows from how that went. */
static void start_chunk(Cpu *c, LoadRequest *r) {
    int32_t res = os_io_read_async(c, r->file.fd, CHUNK_BUFFER, r->chunk_sectors << 11);
    if ((uint32_t)res == PSP_ERR_ASYNC_BUSY) r->status = ST_RETRY_READ;
    else r->status = res != 0 ? ST_RETRY_OPEN : ST_CHUNK_DONE;
}

/* How much of the entry the next chunk takes. */
static void size_chunk(LoadRequest *r) {
    if (r->entry_left < CHUNK_BYTES) {
        r->chunk_sectors = (uint32_t)r->sectors_left;
        r->chunk_bytes = (uint32_t)r->entry_left;
    } else {
        r->chunk_sectors = CHUNK_SECTORS;
        r->chunk_bytes = CHUNK_BYTES;
    }
    r->entry_done += r->chunk_bytes;
}

/* ---------------------------------------------------------------- the states */

static int32_t st_open(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    CALL1(f_089E64BC, request);
    *W(ASYNC_STAGING) = 0;
    if (loader_file_open_quiet(c, request, request_name()) < 0) { r->status = ST_OPEN; return 0; }
    r->status = ST_READ_HEADER;
    *U8(LOAD_IDLE) = 0;
    return 2;
}

static int32_t st_read_header(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    if (r->kind != 0) {
        /* a plain file: a header with one entry of kind 0 and the file's size */
        gaddr stat = c->r[29] + 16;                 /* SceIoStat on the guest stack */
        if (os_io_getstat(c, request_name(), stat) != 0) { r->status = ST_OPEN; return 0; }
        uint32_t *h = W(HEADER_BUFFER);
        h[0] = 1; h[1] = 0;
        r->header_cursor = HEADER_BUFFER + 8;
        r->entries_left = 1;
        h[2] = 0;
        uint32_t size = W(stat)[2];                 /* st_size, low word */
        r->data_size = size;
        h[3] = size;
        r->status = ST_ENTRY_START;
        *U8(LOAD_IDLE) = 0;
        return 2;
    }
    int32_t res = os_io_read_async(c, r->file.fd, HEADER_BUFFER, SECTOR);
    if ((uint32_t)res == PSP_ERR_ASYNC_BUSY) return 0;
    if (res != 0) { r->status = ST_OPEN; return 0; }
    r->status = ST_HEADER_DONE;
    r->header_cursor = HEADER_BUFFER;
    if (r->fighter_heap == 0) *W(FIGHTER_HEAP_USED) = 0;
    else if (r->fighter_heap == 1) *W(FIGHTER_HEAP_USED) = FIGHTER_HEAP_1;
    else if (r->fighter_heap == 2) *W(FIGHTER_HEAP_USED) = FIGHTER_HEAP_2;
    *U8(LOAD_IDLE) = 0;
    return 2;
}

static int32_t st_header_done(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    gaddr result = c->r[29] + 16;
    int32_t res = os_io_poll_async(c, r->file.fd, result);
    if (res == 1 || (uint32_t)res == PSP_ERR_ASYNC_BUSY) return 0;
    if (res != 0) { r->status = ST_OPEN; return 0; }
    if (read_short(result)) { r->status = ST_OPEN; return 0; }
    r->entries_left = *W(r->header_cursor); r->header_cursor += 4;
    r->data_size = *W(r->header_cursor) - SECTOR; r->header_cursor += 4;
    r->status = ST_ENTRY_START;
    r->file_pos = SECTOR;
    return 0;
}

/* Where the entry goes; the loader thread's version of pac_load's entry_destination. */
static void entry_destination(Cpu *c, LoadRequest *r, uint32_t type) {
    uint32_t kind = type & 0xFFFF0000u, lo = type & 0xFF, mid = (type >> 8) & 0xFF;
    uint32_t size = (uint32_t)r->entry_left;
    switch (kind) {
    case KIND_TEXTURE_B:
    case KIND_TEXTURE: {
        gaddr page = W(TEXTURE_PAGES)[r->texture_page + (int32_t)mid];
        gaddr slot = *W(page) + lo * 12;
        if (kind == KIND_TEXTURE_B) W(slot)[1] |= 0x20000000u;
        else W(slot)[1] &= 0xDFFFFFFFu;
        if (*W(ASYNC_STAGING) == 0) *W(ASYNC_STAGING) = CALL1(f_089E04EC, TEXTURE_STAGING);
        r->dest = *W(ASYNC_STAGING);
        CALL5(f_089E0DE0, TEXTURE_LIST, r->dest, 0, 0, 0);
        W(slot)[0] = TEXTURE_LIST;
        if ((r->type & 0xFF) == 0) CALL1(f_089E0CF4, page);
        return;
    }
    case KIND_FIGHTER_A: case KIND_FIGHTER_B: case KIND_FIGHTER_C: {
        gaddr t1 = kind == KIND_FIGHTER_A ? FIGHTER_A_TABLE : kind == KIND_FIGHTER_B ? FIGHTER_B_TABLE : FIGHTER_C_TABLE;
        gaddr t2 = kind == KIND_FIGHTER_A ? FIGHTER_A_TABLE2 : kind == KIND_FIGHTER_B ? FIGHTER_B_TABLE2 : FIGHTER_C_TABLE2;
        gaddr at = *W(FIGHTER_HEAP_USED) + FIGHTER_HEAP;
        r->dest = at;
        W(t1)[r->fighter_slot] = at;
        W(t2)[r->fighter_heap] = at;
        return;
    }
    case KIND_TEXT:     r->dest = W(TEXT_TABLE)[type & 0xF]; return;
    case KIND_DATA:     if (r->kind == 0) r->dest = W(DATA_TABLE)[r->data_slot + (int32_t)lo]; return;   /* a plain file: where queue_load said */
    case KIND_80000000: r->dest = W(DATA_TABLE)[mid]; return;
    case KIND_08000000: r->dest = W(TABLE_08000000)[mid]; return;
    case KIND_PALETTE:  r->dest = W(PALETTE_TABLE)[lo]; return;
    case KIND_02000000: r->dest = W(TABLE_02000000)[lo]; return;
    case KIND_01000000: r->dest = W(TABLE_01000000)[lo]; return;
    case KIND_VOICE: {
        const uint32_t *bank = W(SOUND_BANKS + lo * 8);
        r->dest = CALL4(f_089E2E28, bank[1], (uint32_t)s8(bank[0] + (uint32_t)r->sound_bank), size, (uint32_t)s8((type >> 15) & 1));
        return;
    }
    case KIND_SOUND:
        if (type & 0x80) r->dest = CALL3(f_089E2AA0, (uint32_t)s8(type & 0x7F), (uint32_t)s8(mid), size);
        else r->dest = CALL4(f_089E2E28, SOUND_SMPB, (uint32_t)s8(lo), size, (uint32_t)s8((type >> 15) & 1));
        return;
    default:            /* (0x00020000 is not handled by the loader thread) */
        return;
    }
}

static int32_t st_entry_start(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    r->type = *W(r->header_cursor); r->header_cursor += 4;
    r->entry_left = (int32_t)*W(r->header_cursor); r->header_cursor += 4;
    entry_destination(c, r, r->type);
    r->entry_done = 0;
    r->sectors_left = (r->entry_left + SECTOR - 1) >> 11;
    r->chunk_bytes = 0;
    r->chunk_sectors = 0;
    size_chunk(r);
    start_chunk(c, r);
    return 0;
}

/* An entry has all arrived: hand it over. */
static void entry_finished(Cpu *c, LoadRequest *r) {
    uint32_t type = r->type, kind = type & 0xFFFF0000u;
    switch (kind) {
    case KIND_VOICE:
    case KIND_SOUND:
        r->status = ST_SOUND;
        break;
    case KIND_TEXTURE:
    case KIND_TEXTURE_B: {
        gaddr page = W(TEXTURE_PAGES)[r->texture_page + (int32_t)((type & 0xFF00) >> 8)];
        gaddr staging = *W(ASYNC_STAGING);
        if (*W(staging) == MAGIC_GBIX) {
            gaddr slot = *W(page) + (type & 0xFF) * 12;
            W(slot)[2] = CALL1(f_089E055C, W(staging)[2] + (uint32_t)(int32_t)r->texture_index);
        }
        CALL1(f_089E0CF4, page);
        CALL1(f_089E0E70, r->type & 0xFF);
        break;
    }
    case KIND_FIGHTER_A: case KIND_FIGHTER_B: case KIND_FIGHTER_C: {
        uint32_t *used = W(FIGHTER_HEAP_USED);
        *used = *used + r->entry_done + 3;
        *used &= ~3u;
        break;
    }
    }
    r->entries_left--;
    if (r->status == ST_CHUNK_DONE) r->status = r->entries_left == 0 ? ST_FINISH : ST_ENTRY_START;
}

static int32_t st_chunk_done(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    gaddr result = c->r[29] + 16;
    int32_t res = os_io_poll_async(c, r->file.fd, result);
    if (res == 1 || (uint32_t)res == PSP_ERR_ASYNC_BUSY) return 0;
    if (res != 0 || read_short(result)) { r->status = ST_RETRY_OPEN; return 0; }

    r->sectors_left -= (int32_t)r->chunk_sectors;
    r->staging = CHUNK_BUFFER;
    CALL3(f_089E0DC4, r->dest, CHUNK_BUFFER, r->chunk_bytes);
    r->dest += r->chunk_bytes;
    r->entry_left -= (int32_t)r->chunk_bytes;
    r->file_pos += r->chunk_sectors << 11;
    if (r->entry_left > 0) {
        size_chunk(r);
        start_chunk(c, r);
        return 0;
    }
    entry_finished(c, r);
    return 2;
}

static int32_t st_sound(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    r->status = ST_SOUND_WAIT;
    CALL1(f_089E1D3C, SOUND_HANDLE);
    CALL1(f_089E1D94, 1);
    CALL5(f_089E1D9C, *W(SOUND_HANDLE), r->dest - r->entry_done, r->entry_done, SOUND_CALLBACK, request);
    const uint32_t *bank = W(SOUND_BANKS + (r->type & 0xFF) * 8);
    CALL3(f_089E2B60, *W(SOUND_HANDLE), bank[1], (uint32_t)s8(bank[0] + (uint32_t)r->sound_bank));
    return 0;
}

/* Called by the sound library once it has the bank. */
static void sound_taken(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    CALL1(f_089E1E40, *W(SOUND_HANDLE));
    r->status = r->entries_left != 0 ? ST_ENTRY_START : ST_FINISH;
}

static int32_t st_finish(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    r->status = ST_OPEN;
    U8(LOADER_STATE)[7] = 0;
    *(uint16_t *)(U8(LOADER_STATE) + 2) = 0;
    loader_file_close(c, request);

    /* free the tables this archive replaces (count 0 would walk 2^32 entries, as on the PSP) */
    uint32_t left = W(HEADER_BUFFER)[0];
    gaddr entry = HEADER_BUFFER + 8;
    do {
        if ((*W(entry) & 0xFFFF0000u) & 0x08000000u) {
            uint32_t i = (*W(entry) & 0xFF00) >> 8;
            if (W(TABLE_08000000)[i] != 0) {
                CALL1(f_089E0540, W(TABLE_08000000)[i]);
                W(TABLE_08000000)[(*W(entry) & 0xFF00) >> 8] = 0;
            }
        }
        left--;
        entry += 8;
    } while (left != 0);
    if (*W(ASYNC_STAGING) != 0) { CALL1(f_089E0540, *W(ASYNC_STAGING)); *W(ASYNC_STAGING) = 0; }

    /* release the request, and note whether the whole ring is idle */
    uint8_t *slot = U8(CURRENT_SLOT);
    W(REQUEST_NAMES + *slot * 36u)[0] = 0;
    *slot = (uint8_t)((*slot + 1) & 0xF);
    U8(LOADER_STATE)[4] = 1;
    *U8(LOAD_IDLE) = 0;
    uint8_t all_idle = U8(LOADER_STATE)[4];
    for (int i = 0; i < 16; i++)
        if (W(REQUEST_NAMES + 36u * (uint32_t)i)[0] != 0) { all_idle = 0; *U8(LOAD_IDLE) = 0; }
    U8(LOADER_STATE)[4] = all_idle;
    return 0;
}

static int32_t st_retry_open(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    if (loader_file_open(c, request, request_name()) < 0) return 0;
    if ((int32_t)os_io_lseek(c, r->file.fd, (int64_t)(int32_t)r->file_pos, 0) < 0) return 0;
    int32_t res = os_io_read_async(c, r->file.fd, CHUNK_BUFFER, r->chunk_sectors << 11);
    if ((uint32_t)res == PSP_ERR_ASYNC_BUSY) r->status = ST_RETRY_READ;
    else if (res == 0) r->status = ST_CHUNK_DONE;
    return 0;
}

static int32_t st_retry_read(Cpu *c, gaddr request) {
    LoadRequest *r = guest_ptr(request);
    int32_t res = os_io_read_async(c, r->file.fd, CHUNK_BUFFER, r->chunk_sectors << 11);
    if ((uint32_t)res == PSP_ERR_ASYNC_BUSY) return 0;
    r->status = res != 0 ? ST_RETRY_OPEN : ST_CHUNK_DONE;
    return 0;
}

/* ---------------------------------------------------------------- the thread */

/* One step of the first request that is waiting (through the state table, as the PSP code does). */
static void loader_step(Cpu *c) {
    COVERAGE(0880777C);
    for (uint32_t i = 0; i < 16; i++) {
        if (W(REQUEST_NAMES + 36u * i)[0] == 0) continue;
        *U8(CURRENT_SLOT) = (uint8_t)i;
        gaddr request = loader_request_addr(i);
        c->r[4] = request;
        rt_call_indirect(c, W(STATE_TABLE)[loader_request(i)->status]);
        U8(LOADER_STATE)[7] = (uint8_t)c->r[2];
        return;
    }
}

/* The step functions keep their PSP stack frames: three of them hand the address of a local (a
   SceIoStat, a 64-bit result) to the OS. */
#define WITH_FRAME(size, expr) do { c->r[29] -= (size); RETURN(expr); c->r[29] += (size); } while (0)

SFA_FUNC(08807810, game_loader_open)        { RETURN(st_open(c, ARG(0))); }
SFA_FUNC(08807C18, game_loader_read_header) { WITH_FRAME(0x80, st_read_header(c, ARG(0))); }
SFA_FUNC(08807894, game_loader_header_done) { WITH_FRAME(0x20, st_header_done(c, ARG(0))); }
SFA_FUNC(08808ED4, game_loader_entry_start) { RETURN(st_entry_start(c, ARG(0))); }
SFA_FUNC(088094B8, game_loader_chunk_done)  { WITH_FRAME(0x30, st_chunk_done(c, ARG(0))); }
SFA_FUNC(08807988, game_loader_sound)       { RETURN(st_sound(c, ARG(0))); }
SFA_FUNC(08807A38, game_loader_sound_taken) { sound_taken(c, ARG(0)); }
/* state 5 (0x08807A30, waiting for the sound library) is just `return 0`; not decompiled: never
   reached, the sound library calls back at once */
SFA_FUNC(08807A80, game_loader_finish)      { RETURN(st_finish(c, ARG(0))); }
SFA_FUNC(08807DAC, game_loader_retry_open)  { RETURN(st_retry_open(c, ARG(0))); }
SFA_FUNC(08807E7C, game_loader_retry_read)  { RETURN(st_retry_read(c, ARG(0))); }
SFA_FUNC(0880777C, game_loader_step)        { loader_step(c); }
/* the loader thread's body: a step, then wait for the next frame, for ever */
SFA_FUNC(0880775C, game_loader_thread)      { c->r[29] -= 0x20; for (;;) { loader_step(c); CALL1(f_088054F0, 1); } }
