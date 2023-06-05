#property description Diagonal white wave
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let xpos = iTime * iFrequency;
    let xfreq = (iIntensity + 0.5) * 4.;
    let normCoord = (uv - 0.5) * aspectCorrection;
    let x = ((normCoord.x + normCoord.y) * 0.5 * xfreq + xpos + 10.) % 1.;
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = vec4<f32>(1.) * step(x, 0.3) * smoothstep(0., 0.5, iIntensity);
    return composite(fragColor, c);
}
