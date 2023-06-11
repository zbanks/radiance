#property description Invert the image colors

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let fragColor_rgb = mix(fragColor.rgb, fragColor.a - fragColor.rgb, iIntensity * pow(defaultPulse, 2.));
    return vec4<f32>(fragColor_rgb, fragColor.a);
}
