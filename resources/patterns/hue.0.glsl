void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    hsv.x = mod(hsv.x + iIntensity, 1.0);
    gl_FragColor.rgb = hsv2rgb(hsv);
}
