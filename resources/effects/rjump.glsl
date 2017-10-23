#property description Shift the hue on the beat

void main(void) {
    fragColor = texture(iInput, uv);
    
    float t;
    if (iIntensity < 0.85)
        t = iTime / 4.0;
    else if (iIntensity < 0.95)
        t = iTime / 2.0;
    else
        t = iTime;

    float deviation = mod(3. * floor(t), 8.) / 8.;
    deviation *= clamp(iIntensity / 0.8, 0., 1.);

    vec3 hsv = rgb2hsv(fragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    fragColor.rgb = hsv2rgb(hsv);
}
