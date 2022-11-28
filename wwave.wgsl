//#property description White wave with hard edges
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let xpos = iTime * iFrequency;
    let xfreq = (iIntensity + 0.5) * 2.;
    let x = modulo(normCoord.x * xfreq + xpos, 1.);
    let inputColor = textureSample(iInputsTex[0], iSampler, uv);
    let c = vec4<f32>(1., 1., 1., step(x, 0.3) * smoothstep(0., 0.2, iIntensity));
    return composite(inputColor, premultiply(c));
}
