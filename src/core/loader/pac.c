/* Loading a .pac archive (docs/PAC_FORMAT.md): every entry straight to where the game uses it.
 *
 * pac_load(n) loads archive n of the name table at 0x08A03140 ("st00.pac", "pl00_u.pac", ...) with
 * the next load request's file handle. It reads the 2 KB header, then each entry in order:
 *
 *   1. its type word picks the destination (kind = type & 0xFFFF0000):
 *        textures      a 130 KB staging buffer; the texture slot (page, index) is registered first
 *        sound / voice  a buffer from the SAS sound library, sized for the entry
 *        fighter data  the fighter heap at 0x08E87940 + *0x08E87880, its address kept per fighter
 *        anything else a pointer table per kind (two kinds share one), indexed by the type's low bits
 *   2. the entry is read 32 KB (16 sectors) at a time through a staging buffer and copied there;
 *      a failed read reopens the file, seeks back and tries the chunk again, until it works
 *   3. kinds that need it are then handed over: sound banks and MIDI sequences to the sound
 *      library, textures to the GPU's texture list, and the fighter heap pointer moves past the data
 *
 * Finally the buffers of kinds this archive replaces are freed, and the staging buffer.
 *
 * It returns the archive's data size less 2 KB. Kept exactly: if loading is switched off
 * (0x08E76070) nothing is read, and the PSP code then returns, and frees, whatever two of its
 * registers held (s3 and s7) from the caller: see pac_load_skipped.
 */
#include "../decomp.h"
#include "../psp_os.h"
#include "loader.h"

/* name table and loader state */
#define PAC_NAMES          0x08A03140u   /* const char *[]: the archives, by number */
#define LOADING_OFF        0x08E76070u   /* u16: non-zero: do not load */

/* destinations and kinds: loader.h */

/* functions not decompiled yet */
void f_08809FC8(Cpu *c);   /* before loading (waits for the GPU) */
void f_089E04EC(Cpu *c);   /* allocate, aligned and zeroed */
void f_089E0540(Cpu *c);   /* free */
void f_089E055C(Cpu *c);   /* texture index -> VRAM address */
void f_089E0CF4(Cpu *c);   /* texture page: start */
void f_089E0DC4(Cpu *c);   /* copy */
void f_089E0DE0(Cpu *c);   /* texture list: add */
void f_089E0E70(Cpu *c);   /* texture list: upload slot */
void f_089E1D3C(Cpu *c);   /* sound library: open */
void f_089E1D94(Cpu *c);
void f_089E1D9C(Cpu *c);   /* sound library: give data */
void f_089E1DB4(Cpu *c);   /* sound library: register MIDI sequence */
void f_089E1E40(Cpu *c);   /* sound library: close */
void f_089E2AA0(Cpu *c);   /* sound library: buffer for a sequence */
void f_089E2B60(Cpu *c);   /* sound library: register sample bank */
void f_089E2E28(Cpu *c);   /* sound library: buffer for a bank */

static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t s8(uint32_t v) { return (int8_t)v; }

/* Where the bytes of the entry `type` go (and anything that must be set up before they arrive).
   `dest` is returned unchanged for kinds the loader does not know. */
static gaddr entry_destination(Cpu *c, LoadRequest *r, uint32_t type, uint32_t size, gaddr *texture_staging, gaddr dest) {
    uint32_t kind = type & 0xFFFF0000u;
    uint32_t lo = type & 0xFF, mid = (type >> 8) & 0xFF;
    switch (kind) {
    case KIND_TEXTURE_B:
    case KIND_TEXTURE: {
        gaddr page = W(TEXTURE_PAGES)[r->texture_page + (int32_t)mid];
        gaddr slot = *W(page) + lo * 12;
        if (kind == KIND_TEXTURE_B) W(slot)[1] |= 0x20000000u;
        else W(slot)[1] &= 0xDFFFFFFFu;
        if (*texture_staging == 0) *texture_staging = CALL1(f_089E04EC, TEXTURE_STAGING);
        r->dest = *texture_staging;                 /* (set before the calls, as the PSP code does) */
        CALL5(f_089E0DE0, TEXTURE_LIST, *texture_staging, 0, 0, 0);
        W(slot)[0] = TEXTURE_LIST;
        if ((r->type & 0xFF) == 0) CALL1(f_089E0CF4, page);
        return *texture_staging;
    }
    case KIND_FIGHTER_A: case KIND_FIGHTER_B: case KIND_FIGHTER_C: {
        gaddr table = kind == KIND_FIGHTER_A ? FIGHTER_A_TABLE : kind == KIND_FIGHTER_B ? FIGHTER_B_TABLE : FIGHTER_C_TABLE;
        gaddr at = *W(FIGHTER_HEAP_USED) + FIGHTER_HEAP;
        W(table)[r->fighter_slot] = at;
        return at;
    }
    case KIND_00020000: return W(TABLE_00020000)[type & 0xF];
    case KIND_TEXT:     return W(TEXT_TABLE)[type & 0xF];
    case KIND_DATA:     return W(DATA_TABLE)[r->data_slot + (int32_t)lo];
    case KIND_80000000: return W(DATA_TABLE)[mid];
    case KIND_08000000: return W(TABLE_08000000)[mid];
    case KIND_PALETTE:  return W(PALETTE_TABLE)[lo];
    case KIND_02000000: return W(TABLE_02000000)[lo];
    case KIND_01000000: return W(TABLE_01000000)[lo];
    case KIND_VOICE: {
        const uint32_t *bank = W(SOUND_BANKS + lo * 8);
        return CALL4(f_089E2E28, bank[1], (uint32_t)s8(bank[0] + (uint32_t)r->sound_bank), size, (uint32_t)s8((type >> 15) & 1));
    }
    case KIND_SOUND:
        if (type & 0x80)   /* a sequence */
            return CALL3(f_089E2AA0, (uint32_t)s8(type & 0x7F), (uint32_t)s8(mid), size);
        return CALL4(f_089E2E28, SOUND_SMPB, (uint32_t)s8(lo), size, (uint32_t)s8((type >> 15) & 1));
    default:
        return dest;
    }
}

/* Reads the current entry (r->entry_left bytes) into r->dest, 32 KB at a time. */
static void read_entry(Cpu *c, LoadRequest *r, gaddr request, gaddr name_ptr) {
    r->entry_done = 0;
    r->sectors_left = (r->entry_left + SECTOR - 1) >> 11;
    r->chunk_bytes = 0;
    while (r->entry_left > 0) {
        if (r->entry_left < CHUNK_BYTES) {          /* the rest in one go */
            r->chunk_sectors = (uint32_t)r->sectors_left;
            r->chunk_bytes = (uint32_t)r->entry_left;
        } else {
            r->chunk_sectors = CHUNK_SECTORS;
            r->chunk_bytes = CHUNK_BYTES;
        }
        r->entry_done += r->chunk_bytes;
        while (loader_file_read(c, r->file.fd, CHUNK_BUFFER, r->chunk_sectors << 11) < 0) {
            /* the disc failed: reopen, go back to where this chunk starts, try again */
            do {
                os_delay_thread(c, 100);
                while (loader_file_open(c, request, *W(name_ptr)) < 0) os_delay_thread(c, 100);
            } while ((int32_t)os_io_lseek(c, r->file.fd, (int64_t)(int32_t)r->file_pos, 0) < 0);
        }
        r->sectors_left -= (int32_t)r->chunk_sectors;
        r->staging = CHUNK_BUFFER;
        CALL3(f_089E0DC4, r->dest, CHUNK_BUFFER, r->chunk_bytes);
        r->dest += r->chunk_bytes;
        r->entry_left -= (int32_t)r->chunk_bytes;
        r->file_pos += r->chunk_sectors << 11;
    }
}

/* Hands a finished entry to whoever uses it. `local` is a word on the guest stack for the sound
   library's handle. */
static void entry_loaded(Cpu *c, LoadRequest *r, uint32_t type, gaddr texture_staging, gaddr local) {
    uint32_t kind = type & 0xFFFF0000u;
    gaddr start = r->dest - r->entry_done;
    switch (kind) {
    case KIND_VOICE: {
        CALL1(f_089E1D3C, local);
        CALL1(f_089E1D94, 1);
        gaddr from = (type & 0x7F00) ? W(TABLE_08000000)[(type & 0x7F00) >> 8] : start;
        CALL5(f_089E1D9C, *W(local), from, r->entry_done, 0xFFFFFFFFu, 0);
        const uint32_t *bank = W(SOUND_BANKS + (r->type & 0xFF) * 8);
        CALL3(f_089E2B60, *W(local), bank[1], (uint32_t)s8(bank[0] + (uint32_t)r->sound_bank));
        CALL1(f_089E1E40, *W(local));
        break;
    }
    case KIND_SOUND:
        CALL1(f_089E1D3C, local);
        CALL1(f_089E1D94, 1);
        CALL5(f_089E1D9C, *W(local), start, r->entry_done, 0xFFFFFFFFu, 0);
        if (*W(start) == MAGIC_MIDI)
            CALL3(f_089E1DB4, *W(local), (uint32_t)s8(type & 0x7F), (uint32_t)s8((type >> 8) & 0xFF));
        else
            CALL3(f_089E2B60, *W(local), SOUND_SMPB, (uint32_t)s8(type & 0xFF));
        CALL1(f_089E1E40, *W(local));
        break;
    case KIND_TEXTURE:
    case KIND_TEXTURE_B:
        if (*W(texture_staging) == MAGIC_GBIX) {
            gaddr page = W(TEXTURE_PAGES)[r->texture_page + (int32_t)((type & 0xFF00) >> 8)];
            gaddr slot = *W(page) + (type & 0xFF) * 12;
            uint32_t gbix = W(texture_staging)[2];
            W(slot)[2] = CALL1(f_089E055C, gbix + (uint32_t)(int32_t)r->texture_index);
            CALL1(f_089E0E70, r->type & 0xFF);
        } else {
            CALL1(f_089E0E70, type & 0xFF);
        }
        break;
    case KIND_FIGHTER_A: case KIND_FIGHTER_B: case KIND_FIGHTER_C: {
        uint32_t *used = W(FIGHTER_HEAP_USED);
        *used = *used + r->entry_done + 3;
        *used &= ~3u;
        break;
    }
    }
}

/* Frees what the archive's kinds replace: kinds with 0x08000000 their table entry, sound and voice
   kinds the shared sound buffer. Walks the header; count 0 would walk 2^32 entries, as on the PSP. */
static void free_replaced(Cpu *c) {
    uint32_t left = W(HEADER_BUFFER)[0];
    gaddr entry = HEADER_BUFFER + 8;
    do {
        uint32_t kind = *W(entry) & 0xFFFF0000u;
        if (kind & 0x08000000u) {
            uint32_t *p = &W(TABLE_08000000)[(*W(entry) & 0xFF00) >> 8];
            if (*p != 0) {
                CALL1(f_089E0540, *p);
                W(TABLE_08000000)[(*W(entry) & 0xFF00) >> 8] = 0;
            }
        } else if (kind & 0x60000000u) {
            uint32_t *p = &W(TABLE_08000000)[0];
            if (*p != 0) { CALL1(f_089E0540, *p); *p = 0; }
        }
        left--;
        entry += 8;
    } while (left != 0);
}

static uint32_t pac_load(Cpu *c, uint32_t index, uint32_t caller_s3, uint32_t caller_s7) {
    /* The PSP code's stack frame, kept for the whole call so every function it calls sees the same
       stack; the sound library writes its handle into a word of it. */
    c->r[29] -= 0xA0;
    gaddr local = c->r[29] + 32;
    CALL1(f_08809FC8, index);
    gaddr texture_staging;
    uint32_t result;

    if (*(uint16_t *)guest_ptr(LOADING_OFF) != 0) {
        /* pac_load_skipped: the PSP code goes straight to the clean-up with two registers it never
           set on this path: it returns s3 and frees s7 if non-zero (both the caller's). */
        texture_staging = caller_s7;
        result = caller_s3;
    } else {
        uint32_t slot = *(uint8_t *)guest_ptr(LOAD_REQUEST_NEXT);
        LoadRequest *r = loader_request(slot);
        gaddr request = loader_request_addr(slot);
        gaddr name_ptr = PAC_NAMES + index * 4;
        uint32_t *header = W(HEADER_BUFFER);

        r->file_pos = 0;
        for (;;) {                                  /* open and read the header, until both work */
            while (loader_file_open(c, request, *W(name_ptr)) < 0) os_delay_thread(c, 100);
            if (loader_file_read(c, r->file.fd, HEADER_BUFFER, SECTOR) >= 0) break;
            os_delay_thread(c, 100);
        }
        r->entries_left = header[0];
        r->data_size = header[1] - SECTOR;
        r->file_pos += SECTOR;
        const uint32_t *entry = &header[2];

        texture_staging = 0;
        gaddr dest;
        if (r->entries_left != 0) do {
            r->type = entry[0];
            r->entry_left = (int32_t)entry[1];
            entry += 2;
            uint32_t type = r->type;
            dest = entry_destination(c, r, type, (uint32_t)r->entry_left, &texture_staging, r->dest);
            r->dest = dest;
            read_entry(c, r, request, name_ptr);
            entry_loaded(c, r, r->type, texture_staging, local);
        } while (--r->entries_left != 0);

        loader_file_close(c, request);
        result = r->data_size;
    }

    free_replaced(c);
    if (texture_staging != 0) CALL1(f_089E0540, texture_staging);
    c->r[29] += 0xA0;
    return result;
}

/* u32 pac_load(int archive) */
SFA_FUNC(08808424, game_pac_load) { RETURN(pac_load(c, ARG(0), c->r[19], c->r[23])); }
