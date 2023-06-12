#property description RGB noise

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let rgb = textureSample(iNoiseTex, iSampler, uv).rgb;
    let c = vec4<f32>(rgb, 1.0);
    return mix(fragColor, c, iIntensity * pow(defaultPulse, 0.5));
}
