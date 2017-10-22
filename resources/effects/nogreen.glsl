// Zero out the green channel (green is not a creative color)

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor.g *= 1. - iIntensity;
}
