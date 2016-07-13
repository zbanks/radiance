void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float factor = 1. - 3. * iIntensity * iAudioLow * sawtooth(iTime, 0.1);
    factor = clamp(0.05, 2., factor);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * factor + 0.5);
}
