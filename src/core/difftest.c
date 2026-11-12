/* Differential tests: each listed decompiled function against its translated original, on many
 * random inputs, comparing the result and every byte of the memory it could touch.
 *
 *     a3mx --work work --difftest [N]        (ctest: difftest)
 *
 * This checks functions the parity scenarios do not reach, and corner cases they never hit. It suits
 * functions whose effects stay within their arguments (string, memory and number routines); anything
 * that touches game state or the OS is checked by the parity scenarios instead.
 *
 * To add a function: write a generator that fills the scratch area and sets the arguments, and add a
 * line to CASES.
 */
#include "decomp.h"
#include <stdio.h>
#include <string.h>

uint32_t rt_alloc(uint32_t size, int high, uint32_t align, const char *name);   /* platform/mem.c */
void rt_log(const char *fmt, ...);                                                /* platform/main.c: console and log */

#define SCRATCH_SIZE 0x1000u

/* xorshift: reproducible, so a failure can be replayed by its case number */
typedef struct { uint32_t s; } Rng;
static uint32_t rnd(Rng *r) { uint32_t x = r->s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return r->s = x; }
static uint32_t below(Rng *r, uint32_t n) { return n ? rnd(r) % n : 0; }

static void fill_random(Rng *r, uint8_t *p, uint32_t n) { for (uint32_t i = 0; i < n; i++) p[i] = (uint8_t)rnd(r); }

/* a string of len bytes from a small alphabet (so compares often share prefixes), then a 0 */
static void put_string(Rng *r, uint8_t *p, uint32_t len) {
    static const char alpha[] = "aab9\x80\xff";
    for (uint32_t i = 0; i < len; i++) p[i] = (uint8_t)alpha[below(r, sizeof alpha - 1)];
    p[len] = 0;
}

typedef void (*Gen)(Rng *r, Cpu *c, gaddr scratch, uint8_t *mem);

static void gen_memcpy(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    (void)m;
    uint32_t n = below(r, 200), a = below(r, 1000), b = below(r, 1000);   /* overlaps included */
    c->r[4] = s + 0x400 + a; c->r[5] = s + 0x400 + b; c->r[6] = n;
}
static void gen_memset(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    (void)m; c->r[4] = s + below(r, 2000); c->r[5] = rnd(r); c->r[6] = below(r, 300);
}
static void gen_two_strings(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    uint32_t la = below(r, 40), lb = below(r, 40);
    put_string(r, m + 0x100, la);
    if (below(r, 2)) { memcpy(m + 0x300, m + 0x100, la + 1); if (lb < la && below(r, 2)) m[0x300 + lb] = 0; }
    else put_string(r, m + 0x300, lb);
    c->r[4] = s + 0x100; c->r[5] = s + 0x300; c->r[6] = below(r, 50);
}
static void gen_copy_string(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    put_string(r, m + 0x300, below(r, 60));
    c->r[4] = s + 0x100; c->r[5] = s + 0x300; c->r[6] = below(r, 80);
}
static void gen_one_string(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    put_string(r, m + 0x100, below(r, 80)); c->r[4] = s + 0x100;
}
static void gen_itoa(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    (void)m;
    static const int32_t edge[] = { 0, 1, -1, 9, 10, -10, 0x7FFFFFFF, (int32_t)0x80000000, 255, -256 };
    static const uint32_t bases[] = { 10, 10, 16, 2, 8, 3, 7, 36 };
    c->r[4] = below(r, 4) == 0 ? (uint32_t)edge[below(r, 10)] : rnd(r) >> below(r, 32);
    if (below(r, 2)) c->r[4] = (uint32_t)-(int32_t)c->r[4];
    c->r[5] = s + 0x200; c->r[6] = bases[below(r, 8)];
}
static void gen_round_digits(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    uint32_t len = below(r, 14);
    for (uint32_t i = 0; i < 20; i++) m[0x200 + i] = (uint8_t)(below(r, 3) ? '9' : '0' + below(r, 10));
    c->r[4] = s + 0x201; c->r[5] = below(r, 8) == 0 ? (uint32_t)-(int32_t)below(r, 3) : len;
}

typedef struct { uint32_t addr; const char *name; GuestFn decomp, orig; Gen gen; } Case;

#define CASE(addr, name, gen) { 0x##addr##u, name, f_##addr, orig_f_##addr, gen }
void f_089F3430(Cpu *); void orig_f_089F3430(Cpu *);
void f_089F3470(Cpu *); void orig_f_089F3470(Cpu *);
void f_089F34A4(Cpu *); void orig_f_089F34A4(Cpu *);
void f_089F34D8(Cpu *); void orig_f_089F34D8(Cpu *);
void f_089F351C(Cpu *); void orig_f_089F351C(Cpu *);
void f_089F33E4(Cpu *); void orig_f_089F33E4(Cpu *);
void f_089F3540(Cpu *); void orig_f_089F3540(Cpu *);
void f_089F3604(Cpu *); void orig_f_089F3604(Cpu *);
void f_089F3670(Cpu *); void orig_f_089F3670(Cpu *);
void f_089F35A0(Cpu *); void orig_f_089F35A0(Cpu *);

static const Case CASES[] = {
    CASE(089F3430, "memcpy", gen_memcpy),
    CASE(089F3470, "memset", gen_memset),
    CASE(089F34A4, "strcmp", gen_two_strings),
    CASE(089F34D8, "strcpy", gen_copy_string),
    CASE(089F351C, "strlen", gen_one_string),
    CASE(089F33E4, "memcmp", gen_two_strings),
    CASE(089F3540, "strncpy", gen_copy_string),
    CASE(089F3604, "strrev", gen_one_string),
    CASE(089F3670, "itoa", gen_itoa),
    CASE(089F35A0, "round_digits", gen_round_digits),
};

/* Returns the number of failing functions. */
int decomp_difftest(int iterations) {
    gaddr scratch = rt_alloc(SCRATCH_SIZE, 1, 64, "difftest");
    gaddr stack = rt_alloc(0x4000, 1, 64, "difftest stack");
    if (!scratch || !stack) { rt_log("difftest: no guest memory"); return 1; }
    uint8_t *mem = guest_ptr(scratch);
    static uint8_t before[SCRATCH_SIZE], after_orig[SCRATCH_SIZE];
    int failed = 0;
    for (size_t k = 0; k < sizeof CASES / sizeof CASES[0]; k++) {
        const Case *t = &CASES[k];
        Rng r = { 0x9E3779B9u ^ (uint32_t)(k * 7919) };
        int bad = 0;
        for (int i = 0; i < iterations && !bad; i++) {
            Cpu c0; memset(&c0, 0, sizeof c0);
            fill_random(&r, mem, SCRATCH_SIZE);
            c0.r[29] = stack + 0x3F00; c0.r[31] = 0;
            t->gen(&r, &c0, scratch, mem);
            memcpy(before, mem, SCRATCH_SIZE);

            Cpu a = c0; t->orig(&a);
            memcpy(after_orig, mem, SCRATCH_SIZE);
            memcpy(mem, before, SCRATCH_SIZE);
            Cpu b = c0; t->decomp(&b);

            if (a.r[2] != b.r[2] || memcmp(after_orig, mem, SCRATCH_SIZE) != 0) {
                uint32_t at = 0; while (at < SCRATCH_SIZE && after_orig[at] == mem[at]) at++;
                char where[64] = "";
                if (at < SCRATCH_SIZE) snprintf(where, sizeof where, "; memory differs at +%X (%02X vs %02X)", at, after_orig[at], mem[at]);
                rt_log("difftest FAIL %08X %-14s case %d: args %08X %08X %08X; result %08X vs %08X%s",
                       t->addr, t->name, i, c0.r[4], c0.r[5], c0.r[6], a.r[2], b.r[2], where);
                bad = 1;
            }
        }
        if (bad) failed++;
        else rt_log("difftest ok   %08X %-14s %d cases", t->addr, t->name, iterations);
    }
    rt_log("difftest: %d of %d functions failed", failed, (int)(sizeof CASES / sizeof CASES[0]));
    return failed;
}
