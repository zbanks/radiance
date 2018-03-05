#property description Shift colors away from green (green is not a creative color)

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    float h = hsv.x;
    float parameter = iIntensity * pow(defaultPulse, 2.);
    h = mod(h + 4. / 6., 1.0) - 3. / 6.;
    h *= (1. - parameter / 3.);
    h = mod(h - 1. / 6., 1.0);
    hsv.x = h;
    fragColor.rgb = hsv2rgb(hsv);
}
