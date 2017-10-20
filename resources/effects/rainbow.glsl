// Cycle the color (in HSV) over time

void main(void) {
    fragColor = texture(iInput, uv);

    float deviation;
    deviation = mod(iIntensityIntegral, 1.);

    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    fragColor.rgb = mix(fragColor.rgb, hsv2rgb(hsv), smoothstep(0, 0.2, iIntensity));
}
