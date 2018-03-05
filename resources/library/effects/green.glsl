#property description Zero out the everything but the green channel (green is not a creative color)

void main(void) {
    fragColor = texture(iInput, uv);
    float parameter = iIntensity * pow(defaultPulse, 2.);
    fragColor.r *= 1. - parameter;
    fragColor.b *= 1. - parameter;
}
