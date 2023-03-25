#property description Yellow blob that spins to the beat
#property frequency 1

// I think I ported this correctly; this effect is bad

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let t = iFrequency * iTime * 0.0625 * pi;
    let center = vec2(sin(t), cos(t));
    let center = center * iAudioLevel * 0.9 + 0.1;

    let a = clamp(length(center - normCoord), 0., 1.);
    let a = pow(a, iAudioHi * 3. + 0.1);
    let a = 1.0 - a;
    let a = a * iIntensity;
    let c = vec4(1., 1., 0., 1.) * a;

    return composite(textureSample(iInputsTex[0], iSampler, uv), c);
}

