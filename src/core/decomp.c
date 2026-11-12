/* --reference / --reference-only: run translated originals instead of the hand-written code. */
#include "decomp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct { uint32_t addr; const char *name; const char *file; uint64_t *calls; } DecompEntry;
extern const DecompEntry decomp_table[];   /* generated: build/generated/decomp_table.c */
extern const unsigned decomp_count;

static int all_original;
static uint32_t only[64];
static int n_only;

int decomp_use_original(uint32_t addr) {
    if (all_original) return 1;
    for (int i = 0; i < n_only; i++) if (only[i] == addr) return 1;
    return 0;
}

void decomp_set_reference(void) { all_original = 1; }

int decomp_set_reference_only(const char *list) {    /* "089F3430,089F3470" */
    const char *p = list;
    while (*p && n_only < (int)(sizeof only / sizeof only[0])) {
        char *end;
        unsigned long a = strtoul(p, &end, 16);
        if (end == p) return 0;
        only[n_only++] = (uint32_t)a;
        p = *end == ',' ? end + 1 : end;
    }
    return 1;
}

unsigned decomp_function_count(void) { return decomp_count; }

/* --decomp-report FILE: one line per decompiled function: address, calls this run, name, file */
int decomp_write_report(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    for (unsigned i = 0; i < decomp_count; i++)
        fprintf(f, "%08X %llu %s %s\n", decomp_table[i].addr, (unsigned long long)*decomp_table[i].calls,
                decomp_table[i].name, decomp_table[i].file);
    fclose(f);
    return 1;
}
