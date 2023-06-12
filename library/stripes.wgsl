#property description Dense diagonal white wave with hard edges

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let xpos = iFrequency * iTime;
    let xfreq = (iIntensity + 0.2) * 30.;
    let x = fract((normCoord.x + normCoord.y) * xfreq + xpos);
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = vec4<f32>(1.) * step(x, 0.5) * smoothstep(0., 0.2, iIntensity);
    return composite(fragColor, c);
}
