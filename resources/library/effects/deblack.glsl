#property description Increase alpha ( undo "black" )

void main(void) {
    fragColor = texture(iInput, uv);
    float a = (1. - iIntensity);
    a = max(a, fragColor.a);
    fragColor /= a;
}
