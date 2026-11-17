/* The network screens' whole-screen panels: the rectangles drawn across the screen behind the network
 * menus, messages and their fades (0x088D6BC0, 0x088D6D3C, 0x088E3B8C, 0x088E3BEC, 0x088E3DB0), and the
 * two states of Cammy's CPU routine that pass the screen's width to 0x0895FF6C (0x0895FD00, 0x0895FDB0;
 * docs/PSP_PROGRAM_MAP.md). No scenario reaches 0x088D6D3C or the two states yet.
 *
 * WIDE FORCED: each panel is 480 wide instead of 384 (and 0x0895FF6C gets 480), so it reaches the
 * sides of the 16:9 picture, as Rickow's 16:9 patch (sfa3-PSP-widescreen, MIT) had it
 * (docs/WIDESCREEN.md).
 *
 * The rectangle drawer 0x0880E788 takes six arguments (a0 - t1) and the original leaves registers the
 * callees see, so the calls here set the registers as the original does and call directly.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE FORCED (extensions/widescreen.c; extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t S8(gaddr a) { return (int8_t)*B(a); }
static inline int32_t S16(gaddr a) { return (int16_t)*H(a); }

void f_0880E788(Cpu *c);   /* a flat rectangle: a0 x0, a1 y0, a2 x1, a3 y1 (game pixels), t0 colour, t1 depth */
void f_088DD114(Cpu *c);   /* returns the network session's word at 0x0952390C (2 or more: a third player row) */
void f_088E3B18(Cpu *c);   /* counts a message box's frame (+22) */
void f_0895FE60(Cpu *c);
void f_0895FE7C(Cpu *c);
void f_0895FE94(Cpu *c);
void f_0895FF08(Cpu *c);
void f_0896000C(Cpu *c);
void f_0895FF6C(Cpu *c);   /* a0 the fighter, a1, a2 the screen's width */

#define PANEL_COLOURS 0x08A3EC44u   /* u32[2]: the message panel's colour, by whether the box's +4 is set */

static uint32_t screen_w(void) { return rt_widescreen ? 0x1E0u : 0x180u; }

static void rect(Cpu *c, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t colour, uint32_t depth) {
    c->r[4] = x0; c->r[5] = y0; c->r[6] = x1; c->r[7] = y1; c->r[8] = colour; c->r[9] = depth;
    f_0880E788(c);
}

/* The network menu's backdrop (o the network task's state): with +57, two bars (x 5 - 171 and
   213 - 379) at row +56, and a third (109 - 275) a row lower when 0x088DD114 is 2 or more in a session
   of kind 2 or 3 (+42); with +58, a dark panel from y 20 to 168 (to 144 in such a session). */
static void menu_backdrop(Cpu *c, gaddr o) {
    if (S8(o + 57) != 0) {
        int32_t y = S8(o + 56) + 0x5E;
        uint32_t y0 = (uint32_t)(int16_t)(y + 2), y1 = (uint32_t)(int16_t)(y + 0x18);
        const uint32_t colour = 0xA0800080u;
        rect(c, 5, y0, 0xAB, y1, colour, (uint32_t)S8(o + 57));
        rect(c, 0xD5, y0, 0x17B, y1, colour, (uint32_t)S8(o + 57));
        uint32_t kind = *B(o + 42);
        if (kind == 3 || kind == 2) {
            f_088DD114(c);
            if ((int32_t)c->r[2] >= 2) {
                int32_t y2 = y + 0x29;
                rect(c, 0x6D, (uint32_t)(int16_t)(y2 + 2), 0x113, (uint32_t)(int16_t)(y2 + 0x18), colour,
                     (uint32_t)S8(o + 57));
            }
        }
    }
    if (S8(o + 58) == 0) return;
    uint32_t kind = *B(o + 42), bottom = 0xA8;
    if (kind == 3 || kind == 2) {
        f_088DD114(c);
        if ((int32_t)c->r[2] >= 2) bottom = 0x90;
    }
    rect(c, 0, 0x14, screen_w(), bottom, 0x90000000u, 0x11);
}

/* The network text's black backdrop over the whole screen while +61 is set (o the text's state) */
static void text_backdrop(Cpu *c, gaddr o) {
    if (S8(o + 61) != 0) rect(c, 0, 0, screen_w(), 0xE0, 0xFF000000u, 0x10);
}

/* A message box's panel, y 52 - 172, coloured by a0 (whether the box's +4 is set) */
static void message_panel(Cpu *c, uint32_t second) {
    rect(c, 0, 0x34, screen_w(), 0xAC, *W(PANEL_COLOURS + (second & 0xFF) * 4u), 0x11);
}

/* The panel opening or closing: v (+20) high, centred on the screen's 224 rows */
static void message_panel_part(Cpu *c, gaddr o, int32_t v) {
    int32_t d = 0xE0 - v;
    int16_t top = (int16_t)(d / 2);
    int16_t bottom = (int16_t)(top + v);
    uint32_t second = *W(o + 4) != 0;
    rect(c, 0, (uint32_t)top, screen_w(), (uint32_t)bottom, *W(PANEL_COLOURS + second * 4u), 0x11);
}

/* A message box opening (o its state): +20 grows by 12 a frame up to 120, then +23 is set and the
   panel is drawn whole and the box's frames counted. */
static void message_open(Cpu *c, gaddr o) {
    uint32_t second = *W(o + 4) != 0;
    if (S8(o + 23) == 0) {
        *H(o + 20) = (uint16_t)(S16(o + 20) + 0xC);
        int32_t v = S16(o + 20);
        if (v < 120) { message_panel_part(c, o, v); return; }
        *B(o + 23) = 1;
        *H(o + 20) = 0x78;
    }
    message_panel(c, second);
    c->r[4] = o; f_088E3B18(c);
}

/* A message box closing: its texts (+8, +12, +16) taken down, then +20 shrinks by 12 a frame; when
   below 0 the box is done (+23, +20, +24 cleared). A box that never opened just clears +0 and +4. */
static void message_close(Cpu *c, gaddr o) {
    if (S8(o + 25) == 0) {
        *W(o + 8) = 0;
    } else {
        gaddr p;
        if ((p = *W(o + 12)) != 0) *B(p) = 0;
        if ((p = *W(o + 8)) != 0) *B(p) = 0;
        if ((p = *W(o + 16)) != 0) *B(p) = 0;
        *B(o + 25) = 0;
        *W(o + 8) = 0;
    }
    *W(o + 12) = 0;
    *W(o + 16) = 0;
    if (S8(o + 23) == 0) { *W(o) = 0; *W(o + 4) = 0; return; }
    *H(o + 20) = (uint16_t)(S16(o + 20) - 12);
    int32_t v = S16(o + 20);
    if (v < 0) { *B(o + 23) = 0; *H(o + 20) = 0; *B(o + 24) = 0; return; }
    message_panel_part(c, o, v);
}

/* Two states of Cammy's routines 23 / 24 (o the fighter) that, when 0x0895FE94 says so, pass the
   screen's width on to 0x0895FF6C; otherwise 0x0895FF08 / 0x0896000C. Called directly: the callees
   see what the earlier ones leave in the other registers. */
static void fighter_width(Cpu *c, gaddr o) {
    c->r[4] = o; f_0895FE94(c);
    if ((int32_t)c->r[2] <= 0) return;
    c->r[4] = o; c->r[5] = 0; c->r[6] = screen_w(); f_0895FF6C(c);
}
static void fighter_step_a(Cpu *c, gaddr o) {
    c->r[4] = o; f_0895FE60(c);
    if ((int32_t)c->r[2] < 0) { c->r[4] = o; f_0895FF08(c); return; }
    fighter_width(c, o);
}
static void fighter_step_b(Cpu *c, gaddr o) {
    c->r[4] = o; f_0895FE7C(c);
    if ((int32_t)c->r[2] > 0) { c->r[4] = o; f_0896000C(c); return; }
    fighter_width(c, o);
}

SFA_FUNC(088D6BC0, net_menu_backdrop) { gaddr o = c->r[4]; c->r[29] -= 0x40; menu_backdrop(c, o); c->r[29] += 0x40; }
SFA_FUNC(088D6D3C, net_text_backdrop) { gaddr o = c->r[4]; c->r[29] -= 0x30; text_backdrop(c, o); c->r[29] += 0x30; }
SFA_FUNC(088E3B8C, net_message_panel) { uint32_t s = c->r[4]; c->r[29] -= 0x30; message_panel(c, s); c->r[29] += 0x30; }
SFA_FUNC(088E3BEC, net_message_open) { gaddr o = c->r[4]; c->r[29] -= 0x30; message_open(c, o); c->r[29] += 0x30; }
SFA_FUNC(088E3DB0, net_message_close) { gaddr o = c->r[4]; c->r[29] -= 0x30; message_close(c, o); c->r[29] += 0x30; }
SFA_FUNC(0895FD00, fighter_width_step_a) { gaddr o = c->r[4]; c->r[29] -= 0x20; fighter_step_a(c, o); c->r[29] += 0x20; }
SFA_FUNC(0895FDB0, fighter_width_step_b) { gaddr o = c->r[4]; c->r[29] -= 0x20; fighter_step_b(c, o); c->r[29] += 0x20; }
