#property description Strobe alpha to the beat
#property frequency 1

// TODO: This effect does nothing at frequency 0

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor *= pow(defaultPulse, iIntensity * 5.);
}
