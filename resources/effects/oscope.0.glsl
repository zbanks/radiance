void main(void) {
    gl_FragColor = texture2D(iInput, uv);


    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float x = normCoord.x;
    float wave = 0;
    wave += 0.6 * sin(x * 10. + iTime * 1.) * iAudioLow;
    wave += 0.6 * sin(x * 15. + iTime * -0.3) * iAudioLow;
    wave += 0.2 * sin(x * 40. + iTime * 8.) * iAudioMid;
    wave += 0.2 * sin(x * 70. + iTime * -3.) * iAudioLow;
    wave += 0.1 * sin(x * 120. + iTime * 16.) * iAudioHi;
    wave += 0.1 * sin(x * 180. + iTime * -10.) * iAudioHi;
    wave *= iAudioLevel;
    wave *= smoothstep(0., 0.3, iIntensity);

    float d = abs(normCoord.y - wave);

    float glow = 1. - smoothstep(0, (0.02 + iAudioHi * 0.3) * smoothstep(0., 0.5, iIntensity), d);
    glow += 0.5 * (1. - smoothstep(0, (0.3 + iAudioHi * 0.3) * iIntensity, d));
    vec4 c = vec4(0., 1., 0., 1.) * glow;
    gl_FragColor = composite(gl_FragColor, c);
}

