#property description Zoom out

void main(void) {
    float factor = 1. - iIntensity * pow(defaultPulse, 2.);
    factor = clamp(factor, 0.05, 2.);

    vec2 texcoords = (uv - 0.5) / factor + 0.5;
    fragColor = texture(iInput, texcoords) * box(texcoords);
}
