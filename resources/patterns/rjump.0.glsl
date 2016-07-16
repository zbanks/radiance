void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    
    float t;
    if (iIntensity < 0.85)
        t = iTime / 4.0;
    else if (iIntensity < 0.95)
        t = iTime / 2.0;
    else
        t = iTime;

    float deviation = floor(mod(t, 4.0)) / 4;
    deviation *= clamp(iIntensity / 0.8, 0., 1.);

    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    gl_FragColor.rgb = hsv2rgb(hsv);
}
