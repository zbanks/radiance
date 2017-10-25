#property description Shift the color in HSV space

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.x = mod(hsv.x + iIntensity, 1.0);
    fragColor.rgb = hsv2rgb(hsv);
}
