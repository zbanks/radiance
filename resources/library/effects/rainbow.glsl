#property description Cycle the color (in HSV) over time
#property frequency 0.5

void main(void) {
    fragColor = texture(iInput, uv);

    float deviation;
    deviation = (iFrequency == 0.) ? (iIntensity) : (mod(iFrequency * iTime * 0.5, 1.));

    vec3 hsv = fragColor.rgb;
    hsv = rgb2hsv(hsv);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    hsv = hsv2rgb(hsv);

    fragColor.rgb = mix(fragColor.rgb, hsv, iIntensity);
    fragColor.rgb = (iFrequency == 0.) ? hsv : mix(fragColor.rgb, hsv, iIntensity);
}
