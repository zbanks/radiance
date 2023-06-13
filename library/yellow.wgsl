#property description Yellow and green vertical waves
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    let t = iTime * iFrequency;

    let y = smoothstep(0.2, 0.7, abs(fract(0.5 * (normCoord.x * 4. - t)) * 2. - 1.));
    let g = smoothstep(0.5, 0.9, abs(fract(0.5 * (1. + normCoord.x * 4. - t)) * 2. - 1.));

    let c = vec4<f32>(1., 1., 0., y);
    let c = composite(c, vec4<f32>(0., 1., 0., g * smoothstep(0.5, 0.8, iIntensity)));

    let a = c.a * (smoothstep(0., 0.1, iIntensity));
    let c = vec4<f32>(c.rgb, a);
    let c = clamp(c, vec4<f32>(0.), vec4<f32>(1.));
    return composite(fragColor, premultiply(c));
}
