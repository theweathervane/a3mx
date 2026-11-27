// ge_draw.hlsl for Metal (SDL_GPU's MSL bindings: vertex uniforms buffer(0); fragment texture(0),
// sampler(0), uniforms buffer(0); vertex attributes 0..2). Untested until it runs on a Mac.
#include <metal_stdlib>
using namespace metal;

struct VertexUniforms { float2 fbsize; float2 pad0; };
struct VSIn { float2 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; float4 col [[attribute(2)]]; };
struct VSOut { float4 pos [[position]]; float2 uv; float4 col; };

vertex VSOut vs_main(VSIn i [[stage_in]], constant VertexUniforms &u [[buffer(0)]]) {
    VSOut o;
    o.pos = float4(i.pos.x / u.fbsize.x * 2.0 - 1.0, 1.0 - i.pos.y / u.fbsize.y * 2.0, 0.0, 1.0);
    o.uv = i.uv;
    o.col = i.col;
    return o;
}

struct FragmentUniforms {
    float4 env; float2 texsize; float aref, amask;
    int tex_on, texfunc, rgba, dbl;
    int atest, afunc, clear_on, pad1;
};

fragment float4 fs_main(VSOut i [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]],
                        constant FragmentUniforms &u [[buffer(0)]]) {
    float4 c = i.col;
    if (u.clear_on != 0) return c;
    if (u.tex_on != 0) {
        float4 t = tex.sample(smp, i.uv / u.texsize);
        float4 r;
        if (u.texfunc == 0)      { r.rgb = t.rgb * c.rgb; r.a = u.rgba != 0 ? t.a * c.a : c.a; }
        else if (u.texfunc == 1) { r.rgb = mix(c.rgb, t.rgb, t.a); r.a = c.a; }
        else if (u.texfunc == 2) { r.rgb = mix(c.rgb, u.env.rgb, t.rgb); r.a = u.rgba != 0 ? t.a * c.a : c.a; }
        else if (u.texfunc == 3) { r.rgb = t.rgb; r.a = u.rgba != 0 ? t.a : c.a; }
        else                     { r.rgb = t.rgb + c.rgb; r.a = u.rgba != 0 ? t.a * c.a : c.a; }
        if (u.dbl != 0 && u.texfunc != 3) r.rgb *= 2.0;
        c = saturate(r);
    }
    if (u.atest != 0) {
        float a = floor(c.a * 255.0 + 0.5);
        float x = float(int(a) & int(u.amask)), ref = float(int(u.aref) & int(u.amask));
        bool pass = u.afunc == 1 || (u.afunc == 2 && x == ref) || (u.afunc == 3 && x != ref) || (u.afunc == 4 && x < ref) ||
                    (u.afunc == 5 && x <= ref) || (u.afunc == 6 && x > ref) || (u.afunc == 7 && x >= ref);
        if (!pass) discard_fragment();
    }
    return c;
}
