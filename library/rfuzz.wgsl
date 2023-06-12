#property description Radial fuzz based on audio

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    let a = atan2(normCoord.y, normCoord.x);
    let l = length(normCoord);

    let wave = 0.;
    // As long as all of the multiplicitive factors on "a"
    // are integers, there will be no discontinuities
    let wave = wave + 0.24 * sin(a * 5. + iTime * 1.) * iAudioLow;
    let wave = wave + 0.24 * sin(a * 7. + iTime * -0.3) * iAudioLow;
    let wave = wave + 0.06 * sin(a * 40. + iTime * 8.) * iAudioMid;
    let wave = wave + 0.06 * sin(a * 70. + iTime * -3.) * iAudioLow;
    let wave = wave + 0.03 * sin(a * 120. + iTime * 16.) * iAudioHi;
    let wave = wave + 0.03 * sin(a * 180. + iTime * -10.) * iAudioHi;
    let wave = wave * iAudioLevel;
    let wave = wave * iIntensity;
    let wave = wave * defaultPulse;

    // Avoid discontinuities in the center
    let wave = wave * (smoothstep(0., 0.2, l));

    // Avoid going past the edges
    let extra = 0.5 * (aspectCorrection - 1.);
    let edgeFadeOut = (1. - smoothstep(0.4 + extra, 0.5 + extra, abs(normCoord)));
    let wave = wave * edgeFadeOut.x * edgeFadeOut.y;

    // Move radially by "wave" amount
    let offset = normalize(normCoord) * wave;

    return textureSample(iInputsTex[0], iSampler,  (normCoord + offset) / aspectCorrection + 0.5);
}
