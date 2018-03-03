#property description Strobe green to the beat
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = fragColor;

    float i = pow(defaultPulse, 2.);
    c.r *= 1. - i;
    c.b *= 1. - i;
    c.g *= i;
    fragColor = mix(fragColor, c, iIntensity);
}
