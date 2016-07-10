void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    float deviation = floor(mod(iTime, 4.0)) / 4;
    deviation *=qqiqqq iIntensity;

    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    gl_FragColor.rgb = hsv2rgb(hsv);
}
