// Zoom in (bounce) to the beat & audio

void main(void) {
    float factor = 1. - 3. * iIntensity * iAudioLevel * sawtooth(iTime, 0.1);
    factor = clamp(0.05, 2., factor);

    gl_FragColor = texture2D(iInput, (uv - 0.5) * factor + 0.5);
}
