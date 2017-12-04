#property description Reduce alpha (make input go away)

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor *= (1. - iIntensity);
}
