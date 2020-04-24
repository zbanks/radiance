#property description Radial fuzz based on audio

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float a = atan(normCoord.y, normCoord.x);
    float l = length(normCoord);

    float wave = 0.;
    // As long as all of the multiplicitive factors on "a"
    // are integers, there will be no discontinuities
    wave += 0.24 * sin(a * 5. + iTime * 1.) * iAudioLow;
    wave += 0.24 * sin(a * 7. + iTime * -0.3) * iAudioLow;
    wave += 0.06 * sin(a * 40. + iTime * 8.) * iAudioMid;
    wave += 0.06 * sin(a * 70. + iTime * -3.) * iAudioLow;
    wave += 0.03 * sin(a * 120. + iTime * 16.) * iAudioHi;
    wave += 0.03 * sin(a * 180. + iTime * -10.) * iAudioHi;
    wave *= iAudioLevel;
    wave *= iIntensity;

    // Avoid discontinuities in the center
    wave *= smoothstep(0., 0.2, l);

    // Avoid going past the edges
    vec2 extra = 0.5 * (aspectCorrection - 1.);
    vec2 edgeFadeOut = (1. - smoothstep(0.4 + extra, 0.5 + extra, abs(normCoord)));
    wave *= edgeFadeOut.x * edgeFadeOut.y;

    // Move radially by "wave" amount
    vec2 offset = normalize(normCoord) * wave;

    fragColor = texture(iInput, (normCoord + offset) / aspectCorrection + 0.5);
}

