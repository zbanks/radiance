#property description Base identity pattern for use with `uvmap`

fn main(uv: vec2<f32>) -> vec4<f32> {
    let base = textureSample(iInputsTex[0], iSampler,  uv);
    // The .b channel could be anything; 0.0 plays well with `rainbow`
    let c = vec4<f32>(uv, 0.0, 1.0);
    return mix(base, c, iIntensity * pow(defaultPulse, 2.));
}
