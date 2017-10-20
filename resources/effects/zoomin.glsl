// Zoom in

void main(void) {
    float factor = 1.0 - iIntensity;
    factor = clamp(0.05, 2., factor);

    fragColor = texture(iInput, (uv - 0.5) * factor + 0.5);
}
