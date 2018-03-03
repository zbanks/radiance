#property description Full black strobe
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c;

    float darkness = (iFrequency == 0.) ? pow(iIntensity, 2.) : smoothstep(0., 0.2, iIntensity);

    c = vec4(0., 0., 0., 1.) * pow(defaultPulse, (iIntensity + 0.5) * 2.) * darkness;
    fragColor = composite(fragColor, c);
}
