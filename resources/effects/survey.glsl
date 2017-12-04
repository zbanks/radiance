#property description Zoom in and pan across the surface

void main(void) {
    float t = iTime * 2 *  M_PI / 64.;
    vec2 sweep = vec2(cos(3. * t), sin(2. * t));

    float amount = iIntensity * 0.5;

    float factor = (1. - 2. * amount);

    vec2 newUV = (uv - 0.5) * factor + sweep * amount + 0.5;

    fragColor = texture(iInput, newUV) * box(newUV);
}
