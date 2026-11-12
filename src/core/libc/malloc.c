/* The game's heap: Doug Lea's malloc as shipped in Newlib (mallocr.c, built with MALLOC_ALIGN 16),
 * linked into BOOT.BIN with the rest of the C library.
 *
 * The game's own allocation functions (0x089E04EC aligned + zeroed, 0x089E0540 free) sit on top of
 * memalign and free here, so every task, buffer and object the game creates comes from this code.
 *
 * Layout (all in .data):
 *   0x08AD8588  bin_at(0): the bins. A bin is a fake chunk 8 bytes apart from the next: its fd / bk
 *               (+8 / +12) head a circular list. bin_at(0)->fd is `top`, bin_at(0)->size the bin-block
 *               bitmap, bin_at(1) the last remainder. bin_at(0) itself is the initial top.
 *   0x08AD8990  trim threshold, top pad, sbrk base, max sbrked, max total, sbrked total
 * Chunks: prev_size +0, size +4 (bit 0: previous chunk in use), fd +8, bk +12; user memory at +8.
 *
 * Behaviour is Newlib's exactly, including its 16-byte variant's habit of skipping the odd small bins
 * when scanning, and the lack of a request-size overflow check.
 */
#include "../decomp.h"
#include "../psp_os.h"

/* ---------------------------------------------------------------- the lock (__malloc_lock) */
/* Nested: interrupts go off on the first lock and back on with the last unlock. */
#define LOCK_DEPTH  0x08E758D8u
#define LOCK_STATE  0x08E758DCu

static uint32_t heap_lock(Cpu *c) {
    COVERAGE(089F7BD4);
    uint32_t state = os_suspend_interrupts(c);
    uint32_t *depth = guest_ptr(LOCK_DEPTH);
    if (*depth == 0) *(uint32_t *)guest_ptr(LOCK_STATE) = state;
    (*depth)++;
    return state;
}

static void heap_unlock(Cpu *c) {
    COVERAGE(089F7C10);
    uint32_t *depth = guest_ptr(LOCK_DEPTH);
    if (--*depth == 0) os_resume_interrupts(c, *(uint32_t *)guest_ptr(LOCK_STATE));
}

/* ---------------------------------------------------------------- heap state */
#define BINS            0x08AD8588u
#define TRIM_THRESHOLD  0x08AD8990u
#define TOP_PAD         0x08AD8994u
#define SBRK_BASE       0x08AD8998u
#define MAX_SBRKED      0x08AD899Cu
#define MAX_TOTAL       0x08AD89A0u
#define SBRKED          0x08AD89A4u
#define IMPURE_PTR      0x08AD84DCu   /* struct _reent *_impure_ptr */
#define GLOBAL_ERRNO    0x09526410u   /* errno as _sbrk sets it */

#define MINSIZE          16u
#define PREV_INUSE       1u
#define SIZE_BITS        3u
#define SMALLBIN_LIMIT   504u   /* MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH */
#define MAX_SMALLBIN     63
#define PAGE             0x1000u

typedef struct { uint32_t prev_size, size; gaddr fd, bk; } Chunk;
GUEST_STRUCT_SIZE(Chunk, 16);

static inline Chunk *C(gaddr p) { return guest_ptr(p); }
static inline uint32_t *G(gaddr p) { return guest_ptr(p); }
static inline gaddr bin_at(int32_t i) { return BINS + 8u * (uint32_t)i; }
#define LAST_REMAINDER  bin_at(1)
#define INITIAL_TOP     bin_at(0)
static inline gaddr top(void) { return C(BINS)->fd; }
static inline void set_top(gaddr p) { C(BINS)->fd = p; }
static inline uint32_t *binblocks(void) { return &C(BINS)->size; }
static inline uint32_t chunksize(gaddr p) { return C(p)->size & ~SIZE_BITS; }
static inline void set_inuse_at(gaddr p, uint32_t offset) { C(p + offset)->size |= PREV_INUSE; }
static inline uint32_t block_bit(int32_t idx) { return 1u << ((idx / 4) & 31); }

static void unlink_chunk(gaddr p) {
    gaddr fd = C(p)->fd, bk = C(p)->bk;
    C(fd)->bk = bk;
    C(bk)->fd = fd;
}

static int32_t bin_index(uint32_t size) {
    uint32_t s = size >> 9;
    if (s == 0) return (int32_t)(size >> 3);
    if (s < 5) return (int32_t)(size >> 6) + 56;
    if (s < 21) return (int32_t)s + 91;
    if (s < 85) return (int32_t)(size >> 12) + 110;
    if (s < 341) return (int32_t)(size >> 15) + 119;
    if (s < 1365) return (int32_t)(size >> 18) + 124;
    return 126;
}

/* Puts a free chunk in its bin: small bins are unsorted, large bins kept largest first. */
static void frontlink(gaddr p, uint32_t size) {
    gaddr bk, fd;
    if (size < 512) {
        int32_t idx = (int32_t)(size >> 3);
        *binblocks() |= block_bit(idx);
        bk = bin_at(idx);
        fd = C(bk)->fd;
    } else {
        int32_t idx = bin_index(size);
        bk = fd = bin_at(idx);
        fd = C(bk)->fd;
        if (fd == bk) {
            *binblocks() |= block_bit(idx);
        } else {
            while (fd != bk && size < chunksize(fd)) fd = C(fd)->fd;
            bk = C(fd)->bk;
        }
    }
    C(p)->bk = bk;
    C(p)->fd = fd;
    C(bk)->fd = p;
    C(fd)->bk = p;
}

static void set_last_remainder(gaddr p) {
    gaddr lr = LAST_REMAINDER;
    C(lr)->bk = p; C(lr)->fd = p;
    C(p)->bk = lr; C(p)->fd = lr;
}

/* ---------------------------------------------------------------- sbrk */
void f_08804618(Cpu *c);   /* the game's _sbrk (not decompiled yet) */

/* void *_sbrk_r(struct _reent *r, ptrdiff_t increment) */
static gaddr sbrk_r(Cpu *c, gaddr reent, int32_t increment) {
    COVERAGE(089F8E30);
    *G(GLOBAL_ERRNO) = 0;
    c->r[4] = (uint32_t)increment;
    f_08804618(c);
    gaddr result = c->r[2];
    if (result == 0xFFFFFFFFu && *G(GLOBAL_ERRNO) != 0) *G(reent) = *G(GLOBAL_ERRNO);   /* r->_errno */
    return result;
}

/* ---------------------------------------------------------------- free */
static void malloc_trim_r(Cpu *c, gaddr reent, uint32_t pad);

static void free_r(Cpu *c, gaddr reent, gaddr mem) {
    COVERAGE(089F7310);
    if (mem == 0) return;
    heap_lock(c);
    gaddr p = mem - 8;
    uint32_t hd = C(p)->size;
    uint32_t sz = hd & ~PREV_INUSE;
    gaddr next = p + sz;
    uint32_t nextsz = chunksize(next);

    if (next == top()) {                     /* merge into top */
        sz += nextsz;
        if (!(hd & PREV_INUSE)) {
            uint32_t prevsz = C(p)->prev_size;
            p -= prevsz;
            sz += prevsz;
            unlink_chunk(p);
        }
        C(p)->size = sz | PREV_INUSE;
        set_top(p);
        if (sz >= *G(TRIM_THRESHOLD)) malloc_trim_r(c, reent, *G(TOP_PAD));
        heap_unlock(c);
        return;
    }

    C(next)->size = nextsz;                  /* clear its in-use bit */
    int islr = 0;
    if (!(hd & PREV_INUSE)) {                /* merge backwards */
        uint32_t prevsz = C(p)->prev_size;
        p -= prevsz;
        sz += prevsz;
        if (C(p)->fd == LAST_REMAINDER) islr = 1;
        else unlink_chunk(p);
    }
    if (!(C(next + nextsz)->size & PREV_INUSE)) {   /* merge forwards */
        sz += nextsz;
        if (!islr && C(next)->fd == LAST_REMAINDER) {
            islr = 1;
            set_last_remainder(p);
        } else {
            unlink_chunk(next);
        }
    }
    C(p)->size = sz | PREV_INUSE;
    C(p + sz)->prev_size = sz;
    if (!islr) frontlink(p, sz);
    heap_unlock(c);
}

/* ---------------------------------------------------------------- trim */
/* Gives whole pages at the top back to the system, keeping `pad` bytes. Returns 1 if it released
   anything. */
static int32_t malloc_trim_impl(Cpu *c, gaddr reent, uint32_t pad) {
    COVERAGE(089F75BC);
    heap_lock(c);
    uint32_t top_size = chunksize(top());
    int32_t extra = (int32_t)(((top_size - pad + PAGE - 17) / PAGE - 1) * PAGE);
    if (extra < (int32_t)PAGE) { heap_unlock(c); return 0; }

    gaddr current_brk = sbrk_r(c, reent, 0);
    if (current_brk != top() + top_size) { heap_unlock(c); return 0; }   /* someone else moved it */

    if (sbrk_r(c, reent, -extra) == 0xFFFFFFFFu) {   /* failed: see where the break is now */
        current_brk = sbrk_r(c, reent, 0);
        int32_t top_left = (int32_t)(current_brk - top());
        if (top_left >= (int32_t)MINSIZE) {
            *G(SBRKED) = current_brk - *G(SBRK_BASE);
            C(top())->size = (uint32_t)top_left | PREV_INUSE;
        }
        heap_unlock(c);
        return 0;
    }
    C(top())->size = (top_size - (uint32_t)extra) | PREV_INUSE;
    *G(SBRKED) -= (uint32_t)extra;
    heap_unlock(c);
    return 1;
}
static void malloc_trim_r(Cpu *c, gaddr reent, uint32_t pad) { malloc_trim_impl(c, reent, pad); }

/* ---------------------------------------------------------------- growing the heap */
static void update_peaks(void) {
    uint32_t sbrked = *G(SBRKED);
    if (sbrked > *G(MAX_SBRKED)) *G(MAX_SBRKED) = sbrked;
    if (sbrked > *G(MAX_TOTAL)) *G(MAX_TOTAL) = sbrked;
}

static void malloc_extend_top(Cpu *c, gaddr reent, uint32_t nb) {
    COVERAGE(089F69F4);
    gaddr old_top = top();
    uint32_t old_top_size = chunksize(old_top);
    gaddr old_end = old_top + old_top_size;
    gaddr saved_sbrk_base = *G(SBRK_BASE);
    uint32_t sbrk_size = *G(TOP_PAD) + nb + MINSIZE;
    if (saved_sbrk_base != 0xFFFFFFFFu) sbrk_size = (sbrk_size + PAGE - 1) & ~(PAGE - 1);

    gaddr brk = sbrk_r(c, reent, (int32_t)sbrk_size);
    if (brk == 0xFFFFFFFFu) return;
    if (brk < old_end && old_top != INITIAL_TOP) return;   /* memory below us went elsewhere */

    *G(SBRKED) += sbrk_size;

    if (brk == old_end) {                    /* contiguous: just make top bigger */
        C(top())->size = (old_top_size + sbrk_size) | PREV_INUSE;
        update_peaks();
        return;
    }

    if (*G(SBRK_BASE) == 0xFFFFFFFFu) *G(SBRK_BASE) = brk;
    else *G(SBRKED) += brk - old_end;        /* count the foreign gap as ours */

    /* align the new space's first chunk, and end it on a page boundary */
    uint32_t front_misalign = (brk + 8) & 15u;
    uint32_t align_fix = front_misalign ? 16 - front_misalign : 0;
    brk += align_fix;
    uint32_t page_fix = PAGE - ((brk + sbrk_size) & (PAGE - 1));
    uint32_t correction = align_fix + page_fix;

    gaddr new_brk = sbrk_r(c, reent, (int32_t)correction);
    if (new_brk == 0xFFFFFFFFu) {            /* could not: undo, and leave the heap as it was */
        gaddr unaligned_brk = brk - align_fix;
        *G(SBRKED) -= unaligned_brk - old_end;
        sbrk_r(c, reent, -(int32_t)sbrk_size);
        *G(SBRK_BASE) = saved_sbrk_base;
        return;
    }
    *G(SBRKED) += correction;
    set_top(brk);
    C(brk)->size = (new_brk - brk + correction) | PREV_INUSE;

    if (old_top != INITIAL_TOP) {            /* fence off the old top and free what is left of it */
        if (old_top_size < MINSIZE) {
            C(top())->size = PREV_INUSE;
            return;
        }
        old_top_size = (old_top_size - 12) & ~15u;
        C(old_top)->size = (C(old_top)->size & PREV_INUSE) | old_top_size;
        C(old_top + old_top_size)->size = 4 | PREV_INUSE;
        C(old_top + old_top_size + 4)->size = 4 | PREV_INUSE;
        if (old_top_size >= MINSIZE) free_r(c, reent, old_top + 8);
    }
    update_peaks();
}

/* ---------------------------------------------------------------- malloc */
static uint32_t request_to_size(uint32_t bytes) {
    uint32_t padded = bytes + 4 + 15;
    return (int32_t)padded < (int32_t)(MINSIZE + 15) ? MINSIZE : (padded & ~15u);
}

/* Takes nb bytes off the front of free chunk `victim` (remainder_size left), putting the rest in the
   last-remainder bin. */
static gaddr split_off(gaddr victim, uint32_t nb, uint32_t remainder_size) {
    C(victim)->size = nb | PREV_INUSE;
    gaddr remainder = victim + nb;
    set_last_remainder(remainder);
    C(remainder)->size = remainder_size | PREV_INUSE;
    C(remainder + remainder_size)->prev_size = remainder_size;
    return victim;
}

static gaddr malloc_r(Cpu *c, gaddr reent, uint32_t bytes) {
    COVERAGE(089F6CDC);
    uint32_t nb = request_to_size(bytes);
    gaddr victim;
    int32_t remainder_size, idx;
    heap_lock(c);

    if (nb < SMALLBIN_LIMIT) {               /* small: an exact-fit bin, no searching */
        idx = (int32_t)(nb >> 3);
        gaddr q = bin_at(idx);
        victim = C(q)->bk;
        if (victim != q) {
            uint32_t victim_size = chunksize(victim);
            unlink_chunk(victim);
            set_inuse_at(victim, victim_size);
            heap_unlock(c);
            return victim + 8;
        }
        idx += 2;                            /* (16-byte alignment: the next bin cannot fit either) */
    } else {                                 /* large: best fit within the bin */
        idx = bin_index(nb);
        gaddr bin = bin_at(idx);
        for (victim = C(bin)->bk; victim != bin; victim = C(victim)->bk) {
            uint32_t victim_size = chunksize(victim);
            remainder_size = (int32_t)(victim_size - nb);
            if (remainder_size >= (int32_t)MINSIZE) { idx--; break; }   /* too big: split it later */
            if (remainder_size >= 0) {
                unlink_chunk(victim);
                set_inuse_at(victim, victim_size);
                heap_unlock(c);
                return victim + 8;
            }
        }
        idx++;
    }

    /* the last remainder: split it, use it, or put it back in its bin */
    victim = C(LAST_REMAINDER)->fd;
    if (victim != LAST_REMAINDER) {
        uint32_t victim_size = chunksize(victim);
        remainder_size = (int32_t)(victim_size - nb);
        if (remainder_size >= (int32_t)MINSIZE) {
            split_off(victim, nb, (uint32_t)remainder_size);
            heap_unlock(c);
            return victim + 8;
        }
        C(LAST_REMAINDER)->bk = LAST_REMAINDER;
        C(LAST_REMAINDER)->fd = LAST_REMAINDER;
        if (remainder_size >= 0) {
            set_inuse_at(victim, victim_size);
            heap_unlock(c);
            return victim + 8;
        }
        frontlink(victim, victim_size);
    }

    /* scan the bins with something in them, block by block of four */
    uint32_t block = block_bit(idx);
    if (block <= *binblocks()) {
        if ((block & *binblocks()) == 0) {
            idx = (idx & ~3) + 4;
            block <<= 1;
            while ((block & *binblocks()) == 0) { idx += 4; block <<= 1; }
        }
        for (;;) {
            int32_t start_idx = idx;
            gaddr q = bin_at(idx), bin = q;
            do {
                for (victim = C(bin)->bk; victim != bin; victim = C(victim)->bk) {
                    uint32_t victim_size = chunksize(victim);
                    remainder_size = (int32_t)(victim_size - nb);
                    if (remainder_size >= (int32_t)MINSIZE) {
                        C(victim)->size = nb | PREV_INUSE;
                        unlink_chunk(victim);
                        gaddr remainder = victim + nb;
                        set_last_remainder(remainder);
                        C(remainder)->size = (uint32_t)remainder_size | PREV_INUSE;
                        C(remainder + (uint32_t)remainder_size)->prev_size = (uint32_t)remainder_size;
                        heap_unlock(c);
                        return victim + 8;
                    }
                    if (remainder_size >= 0) {
                        set_inuse_at(victim, victim_size);
                        unlink_chunk(victim);
                        heap_unlock(c);
                        return victim + 8;
                    }
                }
                bin += 8;
                if (idx < MAX_SMALLBIN) { bin += 8; idx++; }   /* odd small bins are always empty */
            } while ((++idx & 3) != 0);

            /* clear the bits of blocks found empty */
            for (;;) {
                if ((start_idx & 3) == 0) { *binblocks() &= ~block; break; }
                q -= 8;
                start_idx--;
                if (C(q)->fd != q) break;
            }
            block <<= 1;
            if (block > *binblocks() || block == 0) break;
            while ((block & *binblocks()) == 0) { idx += 4; block <<= 1; }
        }
    }

    /* the top chunk, grown if need be */
    victim = top();
    remainder_size = (int32_t)(chunksize(victim) - nb);
    if (chunksize(victim) < nb || remainder_size < (int32_t)MINSIZE) {
        malloc_extend_top(c, reent, nb);
        victim = top();
        remainder_size = (int32_t)(chunksize(victim) - nb);
        if (chunksize(victim) < nb || remainder_size < (int32_t)MINSIZE) {
            heap_unlock(c);
            return 0;
        }
    }
    C(victim)->size = nb | PREV_INUSE;
    set_top(victim + nb);
    C(top())->size = (uint32_t)remainder_size | PREV_INUSE;
    heap_unlock(c);
    return victim + 8;
}

/* ---------------------------------------------------------------- memalign */
static gaddr memalign_r(Cpu *c, gaddr reent, uint32_t alignment, uint32_t bytes) {
    COVERAGE(089F776C);
    if (alignment <= 16) return malloc_r(c, reent, bytes);
    uint32_t nb = request_to_size(bytes);

    gaddr m = malloc_r(c, reent, nb + alignment + MINSIZE);   /* room to align and split */
    if (m == 0) return 0;
    heap_lock(c);
    gaddr p = m - 8;
    uint32_t newsize;

    if (m % alignment != 0) {                /* move to an aligned spot inside, give the front back */
        gaddr brk = ((m + alignment - 1) & (0u - alignment)) - 8;
        if ((int32_t)(brk - p) < (int32_t)MINSIZE) brk += alignment;
        uint32_t leadsize = brk - p;
        newsize = chunksize(p) - leadsize;
        C(brk)->size = newsize | PREV_INUSE;
        set_inuse_at(brk, newsize);
        C(p)->size = (C(p)->size & PREV_INUSE) | leadsize;
        free_r(c, reent, p + 8);
        p = brk;
    }

    uint32_t remainder_size = chunksize(p) - nb;   /* give back the tail too */
    if ((int32_t)remainder_size >= (int32_t)MINSIZE) {
        gaddr remainder = p + nb;
        C(remainder)->size = remainder_size | PREV_INUSE;
        C(p)->size = (C(p)->size & PREV_INUSE) | nb;
        free_r(c, reent, remainder + 8);
    }
    heap_unlock(c);
    return p + 8;
}

static gaddr impure(void) { return *G(IMPURE_PTR); }

/* ---------------------------------------------------------------- the game's entry points */
/* __malloc_lock / __malloc_unlock (struct _reent *) */
SFA_FUNC(089F7BD4, game_malloc_lock)   { RETURN(heap_lock(c)); }
SFA_FUNC(089F7C10, game_malloc_unlock) { heap_unlock(c); }

/* void *_malloc_r(struct _reent *, size_t) */
SFA_FUNC(089F6CDC, game_malloc_r) { RETURN(malloc_r(c, ARG(0), ARG(1))); }
/* void _free_r(struct _reent *, void *) */
SFA_FUNC(089F7310, game_free_r) { free_r(c, ARG(0), ARG(1)); }
/* int _malloc_trim_r(struct _reent *, size_t pad) */
SFA_FUNC(089F75BC, game_malloc_trim_r) { RETURN(malloc_trim_impl(c, ARG(0), ARG(1))); }
/* static void malloc_extend_top(struct _reent *, size_t nb) */
SFA_FUNC(089F69F4, game_malloc_extend_top) { malloc_extend_top(c, ARG(0), ARG(1)); }
/* void *_memalign_r(struct _reent *, size_t alignment, size_t bytes) */
SFA_FUNC(089F776C, game_memalign_r) { RETURN(memalign_r(c, ARG(0), ARG(1), ARG(2))); }
/* void *_sbrk_r(struct _reent *, ptrdiff_t) */
SFA_FUNC(089F8E30, game_sbrk_r) { RETURN(sbrk_r(c, ARG(0), (int32_t)ARG(1))); }

/* free and memalign on _impure_ptr. (malloc 0x089F339C and the list free 0x089F2F44 wait for a test
   that runs them: nothing in the scenarios calls them yet.) */
SFA_FUNC(089F33C0, game_free)     { free_r(c, impure(), ARG(0)); }
SFA_FUNC(089F3374, game_memalign) { RETURN(memalign_r(c, impure(), ARG(0), ARG(1))); }
