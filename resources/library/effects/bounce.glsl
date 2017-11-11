#property description Zoom in (bounce) to the beat & audio

void main(void) {
    float factor = 1. - 3. * iIntensity * iAudioLevel * sawtooth(iTime, 0.1);
    factor = clamp(factor, 0.05, 2.);

    fragColor = texture(iInput, (uv - 0.5) * factor + 0.5);
}
