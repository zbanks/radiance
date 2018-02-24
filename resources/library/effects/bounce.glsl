#property description Zoom in (bounce) to the beat & audio
#property frequency 1

void main(void) {
    float factor = 1. - 3. * iIntensity * iAudioLevel * pow(defaultPulse, 2.);
    factor = clamp(factor, 0.05, 2.);

    fragColor = texture(iInput, (uv - 0.5) * factor + 0.5);
}
