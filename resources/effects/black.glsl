// Reduce alpha

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor *= (1. - iIntensity);
}
