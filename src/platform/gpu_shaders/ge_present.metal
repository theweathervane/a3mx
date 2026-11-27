// ge_present.hlsl for Metal. Untested until it runs on a Mac.
#include <metal_stdlib>
using namespace metal;

struct VSIn { float2 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; };
struct VSOut { float4 pos [[position]]; float2 uv; };

vertex VSOut vs_main(VSIn i [[stage_in]]) {
    VSOut o;
    o.pos = float4(i.pos, 0.0, 1.0);
    o.uv = i.uv;
    return o;
}

struct FragmentUniforms { float2 src_size, out_size, uv_max; int sharp, pad0; };

fragment float4 fs_main(VSOut i [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]],
                        constant FragmentUniforms &u [[buffer(0)]]) {
    float2 uv = i.uv;
    if (u.sharp != 0) {
        float2 texel = uv * u.src_size;
        float2 scale = max(floor(u.out_size / u.src_size), float2(1.0));
        float2 base = floor(texel), f = fract(texel) - 0.5;
        float2 range = 0.5 - 0.5 / scale;
        f = (f - clamp(f, -range, range)) * scale + 0.5;
        uv = (base + f) / u.src_size;
    }
    return float4(tex.sample(smp, uv * u.uv_max).rgb, 1.0);
}
