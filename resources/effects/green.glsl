// Zero out the everything but the green channel (green is not a creative color)

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor.r *= 1. - iIntensity;
    fragColor.b *= 1. - iIntensity;
}
