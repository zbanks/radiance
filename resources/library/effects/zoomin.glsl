#property description Zoom in, or bounce to the beat

void main(void) {
    float factor = 1. - iIntensity * pow(defaultPulse, 2.);
    factor = clamp(factor, 0.05, 2.);

    fragColor = texture(iInput, (uv - 0.5) * factor + 0.5);
}
