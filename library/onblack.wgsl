#property description Composite the input image onto black

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let a = mix(fragColor.a, 1.0, iIntensity * pow(defaultPulse, 2.));
    return vec4<f32>(fragColor.rgb, a);
}
