/* The GE's primitives and registers turned into what a renderer backend draws (ge_backend.h, 0.174.0):
 * triangles of GeVtx and an explicit GeDrawState. Every backend gets the same thing, so OpenGL and the
 * SDL3 GPU backend (Vulkan / D3D12 / Metal) can't drift apart in how they read the GE, and none of
 * them reads its registers.
 *
 * Up to 0.173.0 ge_gl.c did this inside its draw; it is moved here unchanged, so OpenGL draws exactly
 * what it drew before:
 *   - primitives into triangles: 3 a list, 4 a strip, 5 a fan, 6 sprites (two corners, the second's
 *     colour for all four, two triangles each); others draw nothing;
 *   - flat shading: every corner of a triangle gets its last vertex's colour;
 *   - the state: R[0xD3] clear; R[0x1E] texture on, R[0xC6] bit 8 bilinear, R[0xC7] clamp s / t,
 *     R[0xC9] texfunc, its alpha and doubling, R[0xCA] its colour; R[0x22] / R[0xDB] the alpha test;
 *     R[0x21] / R[0xDF] / R[0xE0] / R[0xE1] blending (the fixed colours 0xFFFFFF and 0 are ONE and
 *     ZERO; any other is the blend constant, the destination's when both sides have one, as OpenGL has
 *     only one); R[0xD3] bits 8 / 9 (a clear) or R[0xE8] (a draw) the colour mask. */
#include "rt.h"
#include "ge.h"
#include "ge_backend.h"
#include <string.h>

static GeVtx tri[65536 * 3];

static int factor(int f, int src_side, uint32_t fixed, int *uses_const) {
    switch (f) {
    case 0: return src_side ? GE_BF_DST_COLOR : GE_BF_SRC_COLOR;
    case 1: return src_side ? GE_BF_ONE_MINUS_DST_COLOR : GE_BF_ONE_MINUS_SRC_COLOR;
    case 2: case 6: return GE_BF_SRC_ALPHA;
    case 3: case 7: return GE_BF_ONE_MINUS_SRC_ALPHA;
    case 4: case 8: return GE_BF_DST_ALPHA;
    case 5: case 9: return GE_BF_ONE_MINUS_DST_ALPHA;
    default:
        if ((fixed & 0xFFFFFF) == 0xFFFFFF) return GE_BF_ONE;
        if ((fixed & 0xFFFFFF) == 0) return GE_BF_ZERO;
        *uses_const = 1;
        return GE_BF_CONSTANT;
    }
}

void ge_draw(int type, const GeVtx *v, int n, int flat, int sx1, int sy1, int sx2, int sy2) {
    const uint32_t *R = ge_regs();
    GeDrawState s;
    memset(&s, 0, sizeof s);
    s.target = ge_fb_base();
    s.sx1 = sx1; s.sy1 = sy1; s.sx2 = sx2; s.sy2 = sy2;
    s.clear = (R[0xD3] & 1) != 0;

    /* triangles in guest pixels */
    int m = 0;
    switch (type) {
    case 3: for (int i = 0; i + 2 < n; i += 3) { tri[m++] = v[i]; tri[m++] = v[i + 1]; tri[m++] = v[i + 2]; } break;
    case 4: for (int i = 0; i + 2 < n; i++) { tri[m++] = v[i]; tri[m++] = v[i + 1]; tri[m++] = v[i + 2]; } break;
    case 5: for (int i = 1; i + 1 < n; i++) { tri[m++] = v[0]; tri[m++] = v[i]; tri[m++] = v[i + 1]; } break;
    case 6:
        for (int i = 0; i + 1 < n; i += 2) {
            const GeVtx *a = &v[i], *b = &v[i + 1];
            GeVtx q[4] = { *b, *b, *b, *b };                /* sprites take the second vertex's colour */
            q[0].x = a->x; q[0].y = a->y; q[0].u = a->u; q[0].v = a->v;
            q[1].x = b->x; q[1].y = a->y; q[1].u = b->u; q[1].v = a->v;
            q[2].x = a->x; q[2].y = b->y; q[2].u = a->u; q[2].v = b->v;
            q[3].x = b->x; q[3].y = b->y; q[3].u = b->u; q[3].v = b->v;
            tri[m++] = q[0]; tri[m++] = q[1]; tri[m++] = q[2]; tri[m++] = q[1]; tri[m++] = q[3]; tri[m++] = q[2];
        }
        flat = 1;
        break;
    default: m = 0; break;
    }
    if (sx2 < sx1 || sy2 < sy1) m = 0;
    if (!m) { ge_backend->draw(&s, tri, 0); return; }        /* (the backend still makes the target) */

    s.tex_on = !s.clear && (R[0x1E] & 1);
    if (s.tex_on) {
        s.tex.addr = ge_tex_addr(); s.tex.bytes = ge_tex_bytes();
        s.tex.w = ge_tex_w(); s.tex.h = ge_tex_h(); s.tex.stride = ge_tex_stride();
        s.tex.fmt = R[0xC3]; s.tex.size = R[0xB8]; s.tex.mode = R[0xC2];
        s.tex.clut_hash = ge_clut_hash();
        s.tex.decode_rows = ge_decode_texture_rows;
        s.linear = (R[0xC6] >> 8) & 1;
        s.clamp_s = (R[0xC7] & 1) != 0; s.clamp_t = (R[0xC7] & 0x100) != 0;
        uint32_t tf = R[0xC9];
        s.texfunc = (int)(tf & 7); s.tex_alpha = (int)((tf >> 8) & 1); s.doubled = (int)((tf >> 16) & 1);
        s.env = R[0xCA] & 0xFFFFFF;
    }
    s.atest = !s.clear && (R[0x22] & 1);
    if (s.atest) { uint32_t a = R[0xDB]; s.afunc = (int)(a & 7); s.aref = (int)((a >> 8) & 255); s.amask = (int)((a >> 16) & 255); }
    s.blend = !s.clear && (R[0x21] & 1);
    if (s.blend) {
        uint32_t bm = R[0xDF], fa = R[0xE0] & 0xFFFFFF, fb = R[0xE1] & 0xFFFFFF;
        int ua = 0, ub = 0;
        static const int eqs[6] = { GE_BLEND_ADD, GE_BLEND_SUBTRACT, GE_BLEND_REVERSE_SUBTRACT, GE_BLEND_MIN, GE_BLEND_MAX, GE_BLEND_ADD };
        uint32_t eq = (bm >> 8) & 15;
        s.blend_eq = eqs[eq < 6 ? eq : 0];
        s.blend_src = factor((int)(bm & 15), 1, fa, &ua);
        s.blend_dst = factor((int)((bm >> 4) & 15), 0, fb, &ub);
        s.blend_const = ub ? fb : ua ? fa : 0;
    }
    if (s.clear) {
        uint32_t cm = R[0xD3];
        s.write_r = s.write_g = s.write_b = (cm & 0x100) != 0; s.write_a = (cm & 0x200) != 0;
    } else {
        uint32_t cm = R[0xE8];
        s.write_r = (cm & 0xFF) != 0xFF; s.write_g = ((cm >> 8) & 0xFF) != 0xFF; s.write_b = ((cm >> 16) & 0xFF) != 0xFF; s.write_a = 0;
    }

    if (flat)   /* flat shading: every corner gets the triangle's last vertex colour */
        for (int i = 0; i + 2 < m; i += 3) {
            tri[i].r = tri[i + 2].r; tri[i].g = tri[i + 2].g; tri[i].b = tri[i + 2].b; tri[i].a = tri[i + 2].a;
            tri[i + 1].r = tri[i + 2].r; tri[i + 1].g = tri[i + 2].g; tri[i + 1].b = tri[i + 2].b; tri[i + 1].a = tri[i + 2].a;
        }
    ge_backend->draw(&s, tri, m);
}
