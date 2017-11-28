#property description Invert the image lightness, preserving color

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.z = fragColor.a - hsv.z;
    fragColor.rgb = mix(fragColor.rgb, hsv2rgb(hsv), iIntensity);
}
