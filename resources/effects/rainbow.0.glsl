// Cycle the color (in HSV) over time

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);

    float deviation;
    deviation = mod(iIntensityIntegral, 1.);

    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, hsv2rgb(hsv), smoothstep(0, 0.2, iIntensity));
}
