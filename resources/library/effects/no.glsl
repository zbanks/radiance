#property description Reduce alpha (make input go away) or inverse-strobe

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor *= (1. - iIntensity * pow(defaultPulse, 2.));
}
