// The SDL3 GPU backend's presentation (ge_sdlgpu.c): a frame into the window, letterboxed, sharp
// bilinear (whole-number scale with nearest, the rest linear), as ge_gl.c's present shader. Compiled
// by tools/gpu_shaders.py; ge_present.metal is the same for Metal.

struct VSIn {
    [[vk::location(0)]] float2 pos : TEXCOORD0;     // clip space
    [[vk::location(1)]] float2 uv  : TEXCOORD1;
};
struct VSOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

VSOut vs_main(VSIn i) {
    VSOut o;
    o.pos = float4(i.pos, 0.0, 1.0);
    o.uv = i.uv;
    return o;
}

[[vk::combinedImageSampler]] Texture2D<float4> tex : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState smp : register(s0, space2);
cbuffer FragmentUniforms : register(b0, space3) {
    float2 src_size, out_size, uv_max;
    int sharp, pad0;
};

float4 fs_main(VSOut i) : SV_Target0 {
    float2 uv = i.uv;
    if (sharp != 0) {
        float2 texel = uv * src_size;
        float2 scale = max(floor(out_size / src_size), float2(1.0, 1.0));
        float2 base = floor(texel), f = frac(texel) - 0.5;
        float2 range = 0.5 - 0.5 / scale;
        f = (f - clamp(f, -range, range)) * scale + 0.5;
        uv = (base + f) / src_size;
    }
    return float4(tex.Sample(smp, uv * uv_max).rgb, 1.0);
}
