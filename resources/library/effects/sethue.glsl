#property description Set the color in HSV space

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.x = max((iIntensity - 0.1) / 0.9, 0.);
    fragColor.rgb = mix(fragColor.rgb, hsv2rgb(hsv), smoothstep(0., 0.1, iIntensity));
}
