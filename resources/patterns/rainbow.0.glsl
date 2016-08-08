// Cycle the color (in HSV) over time

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    f_color0 = texture2D(iFrame, uv);

    float deviation;
    deviation = mod(iIntensityIntegral, 1.);

    vec3 hsv = rgb2hsv(f_color0.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    f_color0.rgb = mix(f_color0.rgb, hsv2rgb(hsv), smoothstep(0, 0.2, iIntensity));
}
