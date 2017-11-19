#property description Set the hue and saturation equal to the lightness in HSV space

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    float newHue = mod(hsv.x + hsv.z * iIntensity * 30. * (1. - hsv.z), 1.);
    hsv.x = newHue;
//mix(hsv.x, newHue, smoothstep(0., 0.5, iIntensity));
    fragColor.rgb = hsv2rgb(hsv);
}
