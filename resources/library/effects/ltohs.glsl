#property description Set the hue and saturation equal to the lightness in HSV space

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    float parameter = iIntensity * pow(defaultPulse, 0.5);
    hsv.x = mix(hsv.x, hsv.z, smoothstep(0., 0.5, parameter));
    hsv.y = mix(hsv.y, hsv.z, smoothstep(0.5, 1.0, parameter));
    fragColor.rgb = hsv2rgb(hsv);
}
