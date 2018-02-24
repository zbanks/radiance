#property description Shift the color in HSV space

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.x = mod(hsv.x + iIntensity, 1.0);
    vec4 c = vec4(hsv2rgb(hsv), fragColor.a);
    fragColor = mix(fragColor, c, pow(defaultPulse, 2.));
}
