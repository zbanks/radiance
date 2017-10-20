// White slit for testing

void main(void) {
    vec2 onePixel = 1. / iResolution;
    float x = iIntensity;
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    color *= 1. - step(0.5 * onePixel.x, abs(x - uv.x));
    fragColor = texture(iInput, uv);
    fragColor = composite(fragColor, color);
}
