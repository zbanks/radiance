#property description Radiate color from the center based on audio
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(c, fragColor);
}
#buffershader
void main(void) {

    fragColor = texture(iChannel[1], (uv - 0.5) * 0.98 + 0.5);
    fragColor *= exp((iIntensity - 2.) / 50.);
    fragColor = max(fragColor - 0.00001, vec4(0.));

    vec4 c = texture(iInput, uv) * pow(defaultPulse, 2.);
    fragColor = max(fragColor, c);
}
