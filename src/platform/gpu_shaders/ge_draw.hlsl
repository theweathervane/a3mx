// The SDL3 GPU backend's draw (ge_sdlgpu.c): the GE's triangles into a render target, as ge_gl.c's
// shaders draw them (the same texfunc, doubling, alpha test and clear). Compiled by tools/gpu_shaders.py
// into SPIR-V (Vulkan) and DXIL (Direct3D 12): src/platform/gpu_shaders.h. ge_draw.metal is the same for
// Metal. SDL_GPU's binding spaces: vertex uniforms b0 space1; fragment texture / sampler t0 / s0
// space2, uniforms b0 space3. Vertex inputs are TEXCOORD0..2 (locations 0..2).

cbuffer VertexUniforms : register(b0, space1) {
    float2 fbsize;                                  // the render target in guest pixels (512 x 272)
    float2 pad0;
};

struct VSIn {
    [[vk::location(0)]] float2 pos : TEXCOORD0;
    [[vk::location(1)]] float2 uv  : TEXCOORD1;
    [[vk::location(2)]] float4 col : TEXCOORD2;     // UBYTE4_NORM
};
struct VSOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv  : TEXCOORD0;
    [[vk::location(1)]] float4 col : TEXCOORD1;
};

VSOut vs_main(VSIn i) {
    VSOut o;
    // guest row 0 is the target's row 0, at the top (SDL_GPU: +y up in clip space, row 0 at the top)
    o.pos = float4(i.pos.x / fbsize.x * 2.0 - 1.0, 1.0 - i.pos.y / fbsize.y * 2.0, 0.0, 1.0);
    o.uv = i.uv;
    o.col = i.col;
    return o;
}

[[vk::combinedImageSampler]] Texture2D<float4> tex : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState smp : register(s0, space2);
cbuffer FragmentUniforms : register(b0, space3) {
    float4 env;
    float2 texsize;
    float aref, amask;
    int tex_on, texfunc, rgba, dbl;
    int atest, afunc, clear_on, pad1;
};

float4 fs_main(VSOut i) : SV_Target0 {
    float4 c = i.col;
    if (clear_on != 0) return c;
    if (tex_on != 0) {
        float4 t = tex.Sample(smp, i.uv / texsize);
        float4 r;
        if (texfunc == 0)      { r.rgb = t.rgb * c.rgb; r.a = rgba != 0 ? t.a * c.a : c.a; }
        else if (texfunc == 1) { r.rgb = lerp(c.rgb, t.rgb, t.a); r.a = c.a; }
        else if (texfunc == 2) { r.rgb = lerp(c.rgb, env.rgb, t.rgb); r.a = rgba != 0 ? t.a * c.a : c.a; }
        else if (texfunc == 3) { r.rgb = t.rgb; r.a = rgba != 0 ? t.a : c.a; }
        else                   { r.rgb = t.rgb + c.rgb; r.a = rgba != 0 ? t.a * c.a : c.a; }
        if (dbl != 0 && texfunc != 3) r.rgb *= 2.0;
        c = saturate(r);
    }
    if (atest != 0) {
        float a = floor(c.a * 255.0 + 0.5);
        float x = float(int(a) & int(amask)), ref = float(int(aref) & int(amask));
        bool pass = afunc == 1 || (afunc == 2 && x == ref) || (afunc == 3 && x != ref) || (afunc == 4 && x < ref) ||
                    (afunc == 5 && x <= ref) || (afunc == 6 && x > ref) || (afunc == 7 && x >= ref);
        if (!pass) discard;
    }
    return c;
}
