#property description Increase alpha ( undo "black" )
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    float a = (1. - iIntensity * pow(defaultPulse, 2.));
    a = max(a, fragColor.a);
    fragColor /= a;
}
