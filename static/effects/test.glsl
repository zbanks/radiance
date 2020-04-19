#property description A green & red circle in the center

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c;

    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;

    float r = iIntensity;// * (0.7 + 0.3 * pow(defaultPulse, 2.));

    c = vec4(1.) * (1. - smoothstep(r - 0.1, r, length(normCoord)));
    fragColor = composite(fragColor, c);

    c = texture(iChannel[1], (uv - 0.5) / r + 0.5);
    c *= 1. - smoothstep(r - 0.2, r - 0.1, length(normCoord));
    fragColor = composite(fragColor, c);

}
#buffershader
void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;
    fragColor = vec4(abs(normCoord), 0., 1.);
}
