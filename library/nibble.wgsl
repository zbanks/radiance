#property description Transpose high & low bits of each RGB byte

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let v = iIntensity * defaultPulse;

    let nibble_pow2_bits = pow(16., v);

    let scaled = fragColor.rgb * nibble_pow2_bits;
    let lows = fract(scaled);
    let highs = floor(scaled);

    let fragColor_rgb = mix(
        fragColor.rgb,
        lows + highs / 256.,
        smoothstep(0., 0.2, v));
    return add_alpha_dimming(fragColor_rgb, fragColor.a);
}
