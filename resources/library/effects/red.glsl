#property description Change the color to red

void main(void) {
    vec4 c = texture(iInput, uv);
    float parameter = iIntensity * pow(defaultPulse, 2.);
    fragColor.r = mix(c.r, (c.r + c.g + c.b) / 3., parameter);
    fragColor.g = c.g * (1. - parameter);
    fragColor.b = c.b * (1. - parameter);
    fragColor.a = c.a;
    fragColor = clamp(fragColor, 0.0, 1.0);
}
