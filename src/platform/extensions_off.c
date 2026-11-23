/* A build without the extensions (cmake -DA3MX_EXTENSIONS=OFF, or tools/build.py --no-extensions).
 *
 * src/extensions/ is not compiled, tools/data/hooks.txt is ignored (every hooked function is the
 * translated original again) and rt_extensions is 0, so the platform never calls into them. This
 * file only gives the names the platform still refers to something to link against, with the values
 * the game sees without the extensions: no WIDE NATIVE, vsync on, the ad-hoc network
 * up (no virtual players, so only other linked instances answer), saves on the memory stick.
 * None of the functions below is reached while rt_extensions is 0.
 */
#ifdef A3MX_NO_EXTENSIONS
#include <stdint.h>

int rt_widescreen;                       /* WIDE NATIVE stays off */
int rt_vsync = 1;                        /* host_sdl.c */
int rt_frame_timing = 2;                 /* pacing.c: AUTO */
int rt_filter;                           /* shader_gl.c: OFF */
int rt_virtual_psps = 1;                 /* hle_net.c: the network modes as a PSP has them */
int rt_unlock_blocked;                   /* unlocks.h: nothing barred beyond the save */
int rt_human[3] = { 1, -1, -1 };         /* --p2 / --p3: no effect */
int rt_pads_forced;                      /* --pads: no effect */
int store_save_fails;                    /* --save-fails: no effect */
int sysdata_save_questions;              /* --save-questions: the game's questions are always shown */

void settings_load(void) {}
void store_shutdown(void) {}
void netplay_init(void) {}
void netplay_frame(void) {}
void sysdata_frame(void) {}
void cheats_unlocks_unforce(uint8_t *options) { (void)options; }
void quitkey_request(void) {}
uint32_t pcfg_pad_to_psp(int pad, uint32_t raw) { (void)pad; return raw & ~0xC0000000u; }
const char *netplay_name(int node) { (void)node; return "PLAYER1"; }
void sysdata_loaded(const char *file) { (void)file; }
int store_is_ours(const char *file) { (void)file; return 0; }
long store_load(const char *file, uint32_t buf, uint32_t cap) { (void)file; (void)buf; (void)cap; return -1; }
int store_save(const char *file, uint32_t buf, uint32_t n) { (void)file; (void)buf; (void)n; return 0; }
int store_exists(const char *name) { (void)name; return -1; }
int widescreen_edge_shift(uint32_t sp) { (void)sp; return 0; }        /* object/sprite.c */
int widescreen_edge_copy(uint32_t sp, int n) { (void)sp; (void)n; return 0; }
int widescreen_edge_cells(uint32_t sp) { (void)sp; return 0; }        /* object/sprite_drawers.c */
int widescreen_edge_row(uint32_t sp) { (void)sp; return 0; }
int widescreen_beam(uint32_t sp) { (void)sp; return 0; }
int widescreen_text_shift(uint32_t rec) { (void)rec; return 0; }
int widescreen_widen(uint32_t sp) { (void)sp; return 0; }
void widescreen_plain_adjust(uint32_t ob, int *left, int *right) { (void)ob; *left = *right = 0; }
int widescreen_plain_halves(uint32_t ob) { (void)ob; return 0; }
int widescreen_oversize(uint32_t sp) { (void)sp; return 0; }
int widescreen_in_ending(void) { return 0; }
void widescreen_ending_more(void) {}
int widescreen_ending_credits(void) { return 0; }
void widescreen_text_asked(int y) { (void)y; }
int scaled_endings_on(void) { return 0; }
void scaled_endings_scene(int scaled) { (void)scaled; }
void scaled_endings_bare(int bare) { (void)bare; }
int scaled_endings_scene_bare(void) { return 0; }
int scaled_endings_stage_view(uint32_t map, int *shift) { (void)map; *shift = 0; return 0; }
int scaled_endings_oversize(uint32_t sp) { (void)sp; return 0; }
int scaled_endings_profile_shift(uint32_t sp) { (void)sp; return 0; }
int widescreen_column_copies(uint32_t sp, int dx[2]) { (void)sp; (void)dx; return 0; }
int widescreen_bar_ends(uint32_t sp) { (void)sp; return 0; }
int widescreen_piece_shift(uint32_t sp, int dx) { (void)sp; (void)dx; return 0; }
int widescreen_linked_shift(uint32_t sp) { (void)sp; return 0; }
#else
typedef int extensions_off_unused;       /* the extensions are built: nothing here */
#endif
