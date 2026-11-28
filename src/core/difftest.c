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

/* a float worth trying: any bits, a special value, a small whole number, or something near 1 */
static uint32_t some_float(Rng *r) {
    static const uint32_t special[] = { 0, 0x80000000u, 0x3F800000u, 0xBF800000u, 0x7F800000u, 0xFF800000u,
        0x7FC00000u, 0x00000001u, 0x007FFFFFu, 0x00800000u, 0x7F7FFFFFu, 0x3F000000u, 0x40000000u, 0x40400000u,
        0x4B800000u, 0x4D000001u, 0x3F7FFFF7u, 0x3F800008u, 0x43000000u, 0xC3160000u };
    float f; uint32_t b;
    switch (below(r, 6)) {
    case 0: return rnd(r);
    case 1: return special[below(r, sizeof special / sizeof special[0])];
    case 2: f = (float)((int32_t)below(r, 41) - 20); break;
    case 3: f = 1.0f + (float)((int32_t)below(r, 2001) - 1000) / 1.0e6f; break;
    case 4: f = (float)((int32_t)below(r, 200001) - 100000) / 256.0f; break;
    default: return (rnd(r) & 0x807FFFFFu) | ((0x70 + below(r, 0x20)) << 23);   /* middling exponents */
    }
    memcpy(&b, &f, 4); return b;
}
static void gen_floats(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    (void)s; (void)m;
    c->f[12] = some_float(r); c->f[13] = some_float(r);
    c->r[4] = below(r, 4) == 0 ? rnd(r) : (uint32_t)((int32_t)below(r, 601) - 300);
    if (below(r, 16) == 0) c->r[4] = (uint32_t)((int32_t)below(r, 120001) - 60000);
}
/* powf with the library's way of reporting errors (a word of its own, -1 in the game: none) set to
   each of its four values, so the reporting is compared too */
static void gen_powf(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    gen_floats(r, c, s, m);
    *(uint32_t *)guest_ptr(0x08AD8E58u) = below(r, 4) - 1;
}

/* a double worth trying, as two words: any bits, a special value, a float's value, or one whose
   exponent is near the last one's (so sums cancel and round) */
static void some_double(Rng *r, uint32_t *lo, uint32_t *hi) {
    static uint32_t last_exp = 0x3FF;
    static const uint32_t special[][2] = { {0, 0}, {0, 0x80000000u}, {0, 0x3FF00000u}, {0, 0xBFF00000u},
        {0, 0x7FF00000u}, {0, 0xFFF00000u}, {0, 0x7FF80000u}, {1, 0x7FF00000u}, {1, 0}, {0xFFFFFFFFu, 0x000FFFFFu},
        {0, 0x00100000u}, {0xFFFFFFFFu, 0x7FEFFFFFu}, {0, 0x43300000u}, {0, 0xC3300000u}, {0, 0x3FE00000u} };
    double d; uint64_t b;
    switch (below(r, 6)) {
    case 0: *lo = rnd(r); *hi = rnd(r); break;
    case 1: { uint32_t k = below(r, sizeof special / sizeof special[0]); *lo = special[k][0]; *hi = special[k][1]; break; }
    case 2: d = (double)((int32_t)below(r, 200001) - 100000) / 4.0; memcpy(&b, &d, 8); *lo = (uint32_t)b; *hi = (uint32_t)(b >> 32); break;
    case 3: { uint32_t f = some_float(r); float x; memcpy(&x, &f, 4); d = x; memcpy(&b, &d, 8); *lo = (uint32_t)b; *hi = (uint32_t)(b >> 32); break; }
    default:
        *lo = below(r, 3) ? rnd(r) : below(r, 3);
        *hi = (rnd(r) & 0x800FFFFFu) | (((last_exp + below(r, 131) - 65) & 0x7FF) << 20);
        if (below(r, 3) == 0) *hi &= 0xFFF00000u | below(r, 4);
        break;
    }
    last_exp = (*hi >> 20) & 0x7FF;
}
static void gen_doubles(Rng *r, Cpu *c, gaddr s, uint8_t *m) {
    (void)s; (void)m;
    some_double(r, &c->r[4], &c->r[5]);
    some_double(r, &c->r[6], &c->r[7]);
    if (below(r, 12) == 0) { c->r[6] = c->r[4]; c->r[7] = c->r[5] ^ (below(r, 2) << 31); }
}

/* what a case compares besides memory: the result in v0, in v1 too (a double), or in f0 (a float) */
enum { R_V0 = 1, R_V1 = 2, R_F0 = 4 };
typedef struct { uint32_t addr; const char *name; GuestFn decomp, orig; Gen gen; int result; } Case;

#define CASE(addr, name, gen) { 0x##addr##u, name, f_##addr, orig_f_##addr, gen, R_V0 }
#define CASE_AS(addr, name, gen, result) { 0x##addr##u, name, f_##addr, orig_f_##addr, gen, result }
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
void f_089F9F84(Cpu *); void orig_f_089F9F84(Cpu *);
void f_089F94CC(Cpu *); void orig_f_089F94CC(Cpu *);
void f_089FAB48(Cpu *); void orig_f_089FAB48(Cpu *);
void f_089FACCC(Cpu *); void orig_f_089FACCC(Cpu *);
void f_089FAE88(Cpu *); void orig_f_089FAE88(Cpu *);
void f_089FB254(Cpu *); void orig_f_089FB254(Cpu *);
void f_089FB2BC(Cpu *); void orig_f_089FB2BC(Cpu *);
void f_089F9CC0(Cpu *); void orig_f_089F9CC0(Cpu *);
void f_089FB6A8(Cpu *); void orig_f_089FB6A8(Cpu *);
void f_089FBAD8(Cpu *); void orig_f_089FBAD8(Cpu *);
void f_089FAEC0(Cpu *); void orig_f_089FAEC0(Cpu *);
void f_089FBB50(Cpu *); void orig_f_089FBB50(Cpu *);
void f_089FB928(Cpu *); void orig_f_089FB928(Cpu *);
void f_089FBA04(Cpu *); void orig_f_089FBA04(Cpu *);
void f_089F9F64(Cpu *); void orig_f_089F9F64(Cpu *);
void f_089FAC88(Cpu *); void orig_f_089FAC88(Cpu *);
void f_089FACA0(Cpu *); void orig_f_089FACA0(Cpu *);

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
    CASE_AS(089F9F84, "powf_core", gen_floats, R_F0),
    CASE_AS(089F94CC, "powf", gen_powf, R_F0),
    CASE_AS(089FAB48, "sqrtf", gen_floats, R_F0),
    CASE_AS(089FACCC, "scalbnf", gen_floats, R_F0),
    CASE_AS(089FAE88, "copysignf", gen_floats, R_F0),
    CASE_AS(089FB254, "double add", gen_doubles, R_V0 | R_V1),
    CASE_AS(089FB2BC, "double subtract", gen_doubles, R_V0 | R_V1),
    CASE_AS(089F9CC0, "rint", gen_doubles, R_V0 | R_V1),
    CASE_AS(089FB6A8, "double divide", gen_doubles, R_V0 | R_V1),
    CASE_AS(089FBAD8, "double negate", gen_doubles, R_V0 | R_V1),
    CASE_AS(089FAEC0, "float to double", gen_floats, R_V0 | R_V1),
    CASE_AS(089FBB50, "double to float", gen_doubles, R_F0),
    CASE_AS(089FB928, "int to double", gen_doubles, R_V0 | R_V1),
    CASE_AS(089FBA04, "double to int", gen_doubles, R_V0),
    CASE_AS(089F9F64, "float finite", gen_floats, R_V0),
    CASE_AS(089FAC88, "float abs", gen_floats, R_F0),
    CASE_AS(089FACA0, "float nan", gen_floats, R_V0),
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

            int differs = ((t->result & R_V0) && a.r[2] != b.r[2]) || ((t->result & R_V1) && a.r[3] != b.r[3])
                          || ((t->result & R_F0) && a.f[0] != b.f[0]);
            if (differs || memcmp(after_orig, mem, SCRATCH_SIZE) != 0) {
                uint32_t at = 0; while (at < SCRATCH_SIZE && after_orig[at] == mem[at]) at++;
                char where[64] = "";
                if (at < SCRATCH_SIZE) snprintf(where, sizeof where, "; memory differs at +%X (%02X vs %02X)", at, after_orig[at], mem[at]);
                if (t->result & R_F0)
                    rt_log("difftest FAIL %08X %-15s case %d: floats %08X %08X, arg %08X; result %08X vs %08X%s",
                           t->addr, t->name, i, c0.f[12], c0.f[13], c0.r[4], a.f[0], b.f[0], where);
                else
                    rt_log("difftest FAIL %08X %-15s case %d: args %08X %08X %08X %08X; result %08X %08X vs %08X %08X%s",
                           t->addr, t->name, i, c0.r[4], c0.r[5], c0.r[6], c0.r[7], a.r[2], a.r[3], b.r[2], b.r[3], where);
                bad = 1;
            }
        }
        if (bad) failed++;
        else rt_log("difftest ok   %08X %-15s %d cases", t->addr, t->name, iterations);
    }
    rt_log("difftest: %d of %d functions failed", failed, (int)(sizeof CASES / sizeof CASES[0]));
    return failed;
}
