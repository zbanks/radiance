#property description Basic white fill or strobe

void main(void) {
    float pulse = pow(defaultPulse, 2.);
    vec4 white = vec4(1.) * iIntensity * pulse;
    fragColor = composite(texture(iInput, uv), white);
}
