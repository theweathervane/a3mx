/* The game's loader: files, load requests and the .pac archive reader (docs/PAC_FORMAT.md). */
#pragma once
#include "../guest.h"

/* An open file: the descriptor (-1: none) and the path it was opened with. */
typedef struct {
    int32_t fd;
    char    path[128];
} FileHandle;

/* One load request (16 of them, 0x08E76088 + 216 n). The first part is the file; the rest is the
   state of reading a .pac archive entry by entry. */
typedef struct {
    FileHandle file;          /* +0x00 */
    uint8_t  pad84;           /* +0x84 */
    uint8_t  kind;            /* +0x85 what queue_load was given */
    uint8_t  pad86[2];
    gaddr    staging;         /* +0x88 the buffer the last chunk was read into */
    gaddr    dest;            /* +0x8C where the current entry's bytes go (advances) */
    int32_t  entry_left;      /* +0x90 bytes of the current entry still to read */
    int32_t  sectors_left;    /* +0x94 2 KB sectors of it still to read */
    uint32_t entry_done;      /* +0x98 bytes of it read so far */
    uint32_t chunk_bytes;     /* +0x9C bytes copied from the last chunk */
    uint32_t pad_a0;
    uint32_t chunk_sectors;   /* +0xA4 sectors in the last chunk */
    uint32_t file_pos;        /* +0xA8 where the next read starts */
    uint32_t status;          /* +0xAC */
    gaddr    header_cursor;   /* +0xB0 the next entry in the archive header (loader thread) */
    uint32_t data_size;       /* +0xB4 the archive's data size, less 2 KB (returned by the loader) */
    uint32_t pad_b8;
    uint32_t entries_left;    /* +0xBC */
    uint32_t pad_c0;
    uint32_t type;            /* +0xC4 the current entry's type word */
    int8_t   fighter_slot;    /* +0xC8 which fighter's tables the fighter data goes in */
    int8_t   fighter_heap;    /* +0xC9 loader thread: which fighter heap layout, and the second table slot */
    int8_t   texture_page;    /* +0xCA added to an entry's texture page */
    int8_t   sound_bank;      /* +0xCB added to a voice entry's bank */
    int8_t   data_slot;       /* +0xCC added to a misc data entry's slot */
    int8_t   pad_cd[3];
    int16_t  texture_index;   /* +0xD0 added to a texture's GBIX index */
    int16_t  pad_d2;
    uint32_t pad_d4;
} LoadRequest;
GUEST_STRUCT_SIZE(FileHandle, 132);
GUEST_STRUCT_SIZE(LoadRequest, 216);

#define LOAD_REQUESTS 0x08E76088u
#define LOAD_REQUEST_NEXT 0x08E76084u
static inline LoadRequest *loader_request(uint32_t i) { return (LoadRequest *)guest_ptr(LOAD_REQUESTS) + i; }
static inline gaddr loader_request_addr(uint32_t i) { return LOAD_REQUESTS + 216u * i; }

int32_t loader_file_open(Cpu *c, gaddr handle, gaddr name);
int32_t loader_file_open_quiet(Cpu *c, gaddr handle, gaddr name);
void loader_file_close(Cpu *c, gaddr handle);
int32_t loader_file_read(Cpu *c, int32_t fd, gaddr buf, uint32_t size);

/* ---------------------------------------------------------------- where archive entries go */
/* destinations */
#define FIGHTER_HEAP       0x08E87940u   /* fighter data is placed from here ... */
#define FIGHTER_HEAP_USED  0x08E87880u   /* ... plus this many bytes (u32) */
#define FIGHTER_A_TABLE    0x08E75AD8u   /* kind 0x00100000: per fighter slot */
#define FIGHTER_B_TABLE    0x092D9D40u   /* kind 0x00200000 */
#define FIGHTER_C_TABLE    0x092D9E00u   /* kind 0x00400000 */
#define TEXTURE_PAGES      0x08A05530u   /* texture pages: each a pointer to 12-byte slots */
#define TEXTURE_LIST       0x08E76038u
#define TEXTURE_STAGING    0x20800u      /* bytes */
#define DATA_TABLE         0x08A055DCu   /* kinds 0x00000000 and 0x80000000 */
#define SOUND_BANKS        0x08A0558Cu   /* voice banks: { u32 bank, u32 param } by type & 0xFF */
#define TABLE_01000000     0x08A0562Cu
#define TABLE_02000000     0x08A056D4u
#define PALETTE_TABLE      0x08A0577Cu   /* kind 0x04000000 */
#define TABLE_08000000     0x08A05824u   /* kind 0x08000000; [0] also the sound buffer */
#define TEXT_TABLE         0x08A05834u   /* kind 0x00010000 */
#define TABLE_00020000     0x08A0584Cu

#define MAGIC_MIDI   0x6468544Du   /* "MThd" */
#define MAGIC_GBIX   0x58494247u   /* "GBIX" */
#define SOUND_SMPB   0x42504D53u   /* "SMPB": the sound library's tag for a sample bank */

/* entry kinds: type & 0xFFFF0000 */
#define KIND_DATA      0x00000000u
#define KIND_TEXT      0x00010000u
#define KIND_00020000  0x00020000u
#define KIND_FIGHTER_A 0x00100000u
#define KIND_FIGHTER_B 0x00200000u
#define KIND_FIGHTER_C 0x00400000u
#define KIND_TEXTURE_B 0x00800000u   /* a texture flagged 0x20000000 in its slot */
#define KIND_01000000  0x01000000u
#define KIND_02000000  0x02000000u
#define KIND_PALETTE   0x04000000u
#define KIND_08000000  0x08000000u
#define KIND_TEXTURE   0x10000000u
#define KIND_SOUND     0x20000000u
#define KIND_VOICE     0x40000000u
#define KIND_80000000  0x80000000u


#define HEADER_BUFFER      0x08E87080u   /* the archive's 2 KB header */
#define CHUNK_BUFFER       0x08E76E20u   /* reads land here, 32 KB at a time */
#define CHUNK_SECTORS      16
#define CHUNK_BYTES        0x8000
#define SECTOR             0x800
#define REQUEST_NAMES      0x08E86E20u   /* 16 x { u32 busy; char name[32] } */
