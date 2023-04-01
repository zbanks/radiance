#property description Deinterlacing artifacts

fn main(uv: vec2<f32>) -> vec4<f32> {
    let oddEven = floor(uv.x * iResolution.x) % 2. - 0.5; // either 0 or 1

    let pulse = pow(defaultPulse, 2.);

    var offset = vec2<f32>(0.);
    offset.x += oddEven * 0.09 * smoothstep(0.0, 0.5, iIntensity);
    offset.x += oddEven * 0.04 * smoothstep(0.2, 0.6, iIntensity) * rand2(vec2(iTime, uv.y / 1000.)) * pulse;
    offset.x += oddEven * 0.03 * smoothstep(0.4, 0.9, iIntensity) * pulse;
    offset.y += oddEven * 0.05 * smoothstep(0.8, 1.0, iIntensity);

    return textureSample(iInputsTex[0], iSampler, clamp(uv + offset * iIntensity, vec2<f32>(0.), vec2<f32>(1.)));
}
