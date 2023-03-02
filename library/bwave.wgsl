#property description Black sine wave from left to right.
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let x = (normCoord.x + normCoord.y) * 15. + iTime * iFrequency * 3.;
    let c = textureSample(iInputsTex[0], iSampler, uv);
    return c * (1.0 - iIntensity * (0.5 * sin(x) + 0.5));
}

