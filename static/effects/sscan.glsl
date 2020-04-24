#property description White slit for testing

void main(void) {
    float xc = iIntensity;
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    color *= 1. - step(0.5 * onePixel, abs(xc - uv.x));
    fragColor = texture(iInput, uv);
    fragColor = composite(fragColor, color);
}
