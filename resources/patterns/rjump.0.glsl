// Shift the hue on the beat

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);
    
    float t;
    if (iIntensity < 0.85)
        t = iTime / 4.0;
    else if (iIntensity < 0.95)
        t = iTime / 2.0;
    else
        t = iTime;

    float deviation = floor(mod(t, 4.0)) / 4;
    deviation *= clamp(iIntensity / 0.8, 0., 1.);

    vec3 hsv = rgb2hsv(f_color0.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    f_color0.rgb = hsv2rgb(hsv);
}
