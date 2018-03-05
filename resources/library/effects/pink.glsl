#property description Pink polka dots

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c;

    float r = 0.2;

    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float parameter = iIntensity * (0.7 + 0.3 * pow(defaultPulse, 2));

    c = vec4(1., 0.5, 0.5, 1.0);
    c *= 1. - smoothstep(r - onePixel, r, length(mod(normCoord * 5. * parameter - 0.5, 1.) - 0.5));
    c *= smoothstep(0., 0.1, iIntensity);
    fragColor = composite(fragColor, c);
}
