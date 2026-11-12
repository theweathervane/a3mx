/* A build without the extensions (cmake -DA3MX_EXTENSIONS=OFF, or tools/build.py --no-extensions).
 *
 * src/extensions/ is not compiled, tools/data/hooks.txt is ignored (every hooked function is the
 * translated original again) and rt_extensions is 0, so the platform never calls into them. This
 * file only gives the names the platform still refers to something to link against, with the values
 * the game sees without the extensions: no widescreen patch switched on, vsync on, the ad-hoc network
 * up (no virtual players, so only other linked instances answer), saves on the memory stick.
 * None of the functions below is reached while rt_extensions is 0.
 */
#ifdef A3MX_NO_EXTENSIONS
#include <stdint.h>

int rt_widescreen;                       /* the translated widescreen patch stays off */
int rt_vsync = 1;                        /* host_sdl.c */
int rt_frame_timing = 2;                 /* pacing.c: AUTO */
int rt_filter;                           /* shader_gl.c: OFF */
int rt_couch_coop = 1;                   /* hle_net.c: the network modes as a PSP has them */
int rt_human[3] = { 1, -1, -1 };         /* --p2 / --p3: no effect */
int rt_pads_forced;                      /* --pads: no effect */
int store_save_fails;                    /* --save-fails: no effect */
int sysdata_save_questions;              /* --save-questions: the game's questions are always shown */

void settings_load(void) {}
void store_shutdown(void) {}
void netplay_init(void) {}
void netplay_frame(void) {}
void sysdata_frame(void) {}
void quitkey_request(void) {}
uint32_t pcfg_pad_to_psp(int pad, uint32_t raw) { (void)pad; return raw & ~0xC0000000u; }
const char *netplay_name(int node) { (void)node; return "PLAYER1"; }
void sysdata_loaded(const char *file) { (void)file; }
int store_is_ours(const char *file) { (void)file; return 0; }
long store_load(const char *file, uint32_t buf, uint32_t cap) { (void)file; (void)buf; (void)cap; return -1; }
int store_save(const char *file, uint32_t buf, uint32_t n) { (void)file; (void)buf; (void)n; return 0; }
int store_exists(const char *name) { (void)name; return -1; }
#else
typedef int extensions_off_unused;       /* the extensions are built: nothing here */
#endif
