#property description Cycle the color (in HSV) over time

void main(void) {
    fragColor = texture(iInput, uv);

    float deviation;
    deviation = mod(iIntensityIntegral * 0.5, 1.);

    vec4 hsv = demultiply(fragColor);
    hsv.rgb = rgb2hsv(hsv.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    hsv.rgb = hsv2rgb(hsv.rgb);
    hsv = premultiply(hsv);

    fragColor.rgb = mix(fragColor.rgb, hsv.rgb, smoothstep(0., 0.2, iIntensity));
}
