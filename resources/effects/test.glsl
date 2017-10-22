// A green & red circle in the center

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c;

    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;

    c = vec4(1.) * (1. - smoothstep(iIntensity - 0.1, iIntensity, length(normCoord)));
    fragColor = composite(fragColor, c);

    c = texture(iChannel[1], (uv - 0.5) / iIntensity + 0.5);
    c *= 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(normCoord));
    fragColor = composite(fragColor, c);

}
#buffershader
void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;
    fragColor = vec4(abs(normCoord), 0., 1.);
}

#buffershader
void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;
    fragColor = vec4(abs(normCoord), 0., 1.);
}
