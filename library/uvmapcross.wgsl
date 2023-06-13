#property description Use .rg as .uv and crossfade
#property inputCount 2

fn main(uv: vec2<f32>) -> vec4<f32> {
    let map = textureSample(iInputsTex[1], iSampler,  uv);
    let newUV = mix(uv, map.rg, min(iIntensity * 2.0, 1.0) * map.a);
    let mappedColor = textureSample(iInputsTex[0], iSampler,  newUV);
    return mix(mappedColor, map, max(0.0, iIntensity * 2.0 - 1.0));
}
