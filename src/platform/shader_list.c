/* Every filter SCREEN SETTING > FILTER offers (0.166.0; shader.h says what is in it): the favourites
 * (SHADER_FILTERS), the catalogue (one line a preset, tab-separated: category, name, path, flags, ...:
 * the repository's tools/data/shader_catalogue.tsv, made once for the pinned slang-shaders commit, or an
 * a3mx_catalogue.tsv beside the shaders, which an older download made), and the player's own *.slangp
 * in <configs>/shaders/ (CUSTOM). Read once, the first time anything asks (settings_load, --filter, the
 * first frame): file reads only, nothing the game itself sees, so the game plays the same with any of
 * it there or not.
 *
 * Without the shaders downloaded no catalogue is read (nothing in it could be drawn), and the list is
 * the favourites, as before 0.166.0. A preset that turns out not to load is reported when chosen and
 * the picture is shown plain (shader_gl.c). */
#include "rt.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

const char *shader_lib_arg, *shader_dir_arg;   /* main.c: --librashader FILE, --shaders DIR */
extern const char *rt_work_dir;               /* main.c: --work */
extern char rt_config_dir[1024];              /* main.c: the player's configs/ ("": --save-dir, no folder of theirs) */

#define MAX_CATS 48
static FilterEntry *list;
static int n_list, cap_list, loaded, n_catalogued;
static char cats[MAX_CATS][32];
static int n_cats, cat_first[MAX_CATS], cat_size[MAX_CATS];
static char shaders_dir[1100], custom_dir[1100], repo_catalogue[1100];

static int is_dir(const char *p) { struct stat s; return stat(p, &s) == 0 && (s.st_mode & S_IFMT) == S_IFDIR; }

static int category(const char *name) {
    for (int k = 0; k < n_cats; k++) if (!strcmp(cats[k], name)) return k;
    if (n_cats == MAX_CATS) return n_cats - 1;            /* (never: the catalogue has about 30) */
    snprintf(cats[n_cats], sizeof cats[0], "%s", name);
    return n_cats++;
}
static FilterEntry *add(int cat, const char *name, const char *path) {
    if (n_list == cap_list) {
        int cap = cap_list ? cap_list * 2 : 256;
        FilterEntry *l = (FilterEntry *)realloc(list, sizeof *l * (size_t)cap);
        if (!l) return NULL;
        list = l; cap_list = cap;
    }
    FilterEntry *e = &list[n_list++];
    memset(e, 0, sizeof *e);
    e->category = cat;
    snprintf(e->name, sizeof e->name, "%s", name);
    snprintf(e->path, sizeof e->path, "%s", path ? path : "");
    return e;
}

/* where the presets are: --shaders, A3MX_SHADERS_DIR, else .tools/slang-shaders beside the work folder */
static void find_dirs(void) {
    const char *work = rt_work_dir ? rt_work_dir : "work";
    char beside[1100]; snprintf(beside, sizeof beside, "%s", work);
    size_t k = strlen(beside);
    while (k && (beside[k - 1] == '/' || beside[k - 1] == '\\')) beside[--k] = 0;
    while (k && beside[k - 1] != '/' && beside[k - 1] != '\\') beside[--k] = 0;   /* the folder holding work/, with its slash */
    const char *senv = getenv("A3MX_SHADERS_DIR");
    if (shader_dir_arg) snprintf(shaders_dir, sizeof shaders_dir, "%s", shader_dir_arg);
    else if (senv && *senv) snprintf(shaders_dir, sizeof shaders_dir, "%s", senv);
    else snprintf(shaders_dir, sizeof shaders_dir, "%s.tools/slang-shaders", beside);
    snprintf(repo_catalogue, sizeof repo_catalogue, "%stools/data/shader_catalogue.tsv", beside);
    if (rt_config_dir[0]) snprintf(custom_dir, sizeof custom_dir, "%s/shaders", rt_config_dir);
    else custom_dir[0] = 0;
}

static void read_catalogue(void) {
    if (!is_dir(shaders_dir)) return;                     /* (no shaders: nothing in it could be drawn) */
    char p[1200]; snprintf(p, sizeof p, "%s/a3mx_catalogue.tsv", shaders_dir);
    FILE *f = fopen(p, "r");                              /* one an older download made (before 0.171.0) */
    if (!f && (f = fopen(repo_catalogue, "r"))) snprintf(p, sizeof p, "%s", repo_catalogue);   /* the repository's */
    if (!f) { rt_log("filter: no catalogue (%s): the favourites only", repo_catalogue); return; }
    rt_log("filter: catalogue %s", p);
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char *fld[4] = { 0 }; int nf = 0;
        for (char *s = line; nf < 4;) {
            fld[nf++] = s;
            char *t = strchr(s, '\t');
            if (!t) { s[strcspn(s, "\r\n")] = 0; break; }
            *t = 0; s = t + 1;
        }
        if (nf < 3 || !fld[0][0] || !fld[1][0] || !fld[2][0]) continue;
        for (char *s = fld[1]; *s; s++) *s = (char)toupper((unsigned char)*s);
        FilterEntry *e = add(category(fld[0]), fld[1], fld[2]);
        if (e && nf >= 4 && strstr(fld[3], "heavy")) e->heavy = 1;
        if (e) n_catalogued++;
    }
    fclose(f);
}

/* the player's own: every *.slangp in custom_dir and below (a few levels), by name */
static int by_name(const void *a, const void *b) { return strcmp(((const FilterEntry *)a)->name, ((const FilterEntry *)b)->name); }
static void custom_name(const char *file, char *out, size_t n) {
    size_t k = 0;
    for (const char *s = file; *s && *s != '.' && k + 1 < n; s++)
        out[k++] = *s == '_' ? ' ' : (char)toupper((unsigned char)*s);
    out[k] = 0;
}
static void scan(const char *dir, int depth, int cat) {
    if (depth > 3) return;
#ifdef _WIN32
    char pat[1200]; snprintf(pat, sizeof pat, "%s/*", dir);
    WIN32_FIND_DATAA d; HANDLE h = FindFirstFileA(pat, &d);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        const char *nm = d.cFileName;
        int sub = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    DIR *dh = opendir(dir);
    if (!dh) return;
    struct dirent *de;
    while ((de = readdir(dh))) {
        const char *nm = de->d_name;
        char full0[1200]; snprintf(full0, sizeof full0, "%s/%s", dir, nm);
        int sub = is_dir(full0);
#endif
        if (nm[0] == '.') continue;
        char full[1200]; snprintf(full, sizeof full, "%s/%s", dir, nm);
        size_t l = strlen(nm);
        if (sub) scan(full, depth + 1, cat);
        else if (l > 7 && !strcmp(nm + l - 7, ".slangp")) {
            char name[64]; custom_name(nm, name, sizeof name);
            FilterEntry *e = add(cat, name, full);
            if (e) e->custom = 1;
        }
#ifdef _WIN32
    } while (FindNextFileA(h, &d));
    FindClose(h);
#else
    }
    closedir(dh);
#endif
}

void filters_load(void) {
    if (loaded) return;
    loaded = 1;
    find_dirs();
    int fav = category("FAVOURITES");
    for (int k = 0; k < SHADER_BUILTIN_COUNT; k++) {
        FilterEntry *e = add(fav, SHADER_FILTERS[k].name, SHADER_FILTERS[k].preset);
        if (e) e->params = SHADER_FILTERS[k].params;
        if (e && SHADER_FILTERS[k].preset && !strcmp(SHADER_FILTERS[k].preset, "crt/crt-royale.slangp")) e->heavy = 1;
    }
    read_catalogue();
    if (custom_dir[0]) {
        int cat = category("CUSTOM"), from = n_list;
        scan(custom_dir, 0, cat);
        if (n_list > from) qsort(list + from, (size_t)(n_list - from), sizeof *list, by_name);
        else n_cats--;                                     /* (no CUSTOM page with nothing on it) */
    }
    for (int k = 0; k < n_cats; k++) { cat_first[k] = -1; cat_size[k] = 0; }
    for (int i = 0; i < n_list; i++) {                     /* (the catalogue is sorted by category: they are contiguous) */
        int k = list[i].category;
        if (cat_first[k] < 0) cat_first[k] = i;
        cat_size[k]++;
    }
    rt_log("filter: %d filters in %d categories (%d from the catalogue, %d of the player's in %s)", n_list, n_cats, n_catalogued,
           n_list - SHADER_BUILTIN_COUNT - n_catalogued, custom_dir[0] ? custom_dir : "(no folder)");
}

int filter_count(void) { filters_load(); return n_list; }
const FilterEntry *filter_get(int i) { filters_load(); return i >= 0 && i < n_list ? &list[i] : NULL; }
int filter_categories(void) { filters_load(); return n_cats; }
const char *filter_category_name(int k) { filters_load(); return k >= 0 && k < n_cats ? cats[k] : ""; }
int filter_category_first(int k) { filters_load(); return k >= 0 && k < n_cats && cat_first[k] >= 0 ? cat_first[k] : 0; }
int filter_category_size(int k) { filters_load(); return k >= 0 && k < n_cats ? cat_size[k] : 0; }
const char *filters_shaders_dir(void) { filters_load(); return shaders_dir; }
const char *filters_custom_dir(void) { filters_load(); return custom_dir; }
int filters_catalogued(void) { filters_load(); return n_catalogued; }

static const char *base_name(const char *p) {
    const char *b = p;
    for (const char *s = p; *s; s++) if (*s == '/' || *s == '\\') b = s + 1;
    return b;
}
void filter_key(int i, char *out, size_t n) {
    const FilterEntry *e = filter_get(i);
    if (!e || i < SHADER_BUILTIN_COUNT) { snprintf(out, n, "%s", e ? e->name : "OFF"); return; }
    if (e->custom) {                                       /* custom:sub/NAME.slangp, under the player's folder */
        size_t l = strlen(custom_dir);
        snprintf(out, n, "custom:%s", !strncmp(e->path, custom_dir, l) && e->path[l] == '/' ? e->path + l + 1 : base_name(e->path));
        return;
    }
    snprintf(out, n, "%s", e->path);
}
int filter_find(const char *key) {
    filters_load();
    if (!key || !*key) return -1;
    for (int k = 0; k < SHADER_BUILTIN_COUNT && k < n_list; k++) if (!strcmp(key, list[k].name)) return k;
    char kk[1100];
    for (int i = SHADER_BUILTIN_COUNT; i < n_list; i++) { filter_key(i, kk, sizeof kk); if (!strcmp(kk, key)) return i; }
    return -1;
}
/* letters and digits only, in capitals: crt-curved is CRT CURVED */
static void squash(const char *s, char *out, size_t n) {
    size_t k = 0;
    for (; *s && k + 1 < n; s++) { char ch = (char)toupper((unsigned char)*s); if (isalnum((unsigned char)ch)) out[k++] = ch; }
    out[k] = 0;
}
int filter_find_loose(const char *s) {
    int i = filter_find(s);
    if (i >= 0) return i;
    char a[128], b[128]; squash(s, a, sizeof a);
    for (int k = 0; k < SHADER_BUILTIN_COUNT && k < n_list; k++) { squash(list[k].name, b, sizeof b); if (!strcmp(a, b)) return k; }
    for (int k = SHADER_BUILTIN_COUNT; k < n_list; k++) { squash(list[k].name, b, sizeof b); if (!strcmp(a, b)) return k; }
    return -1;
}
