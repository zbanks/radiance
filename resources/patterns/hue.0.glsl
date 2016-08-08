// Shift the color in HSV space

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    f_color0 = texture2D(iFrame, uv);
    vec3 hsv = rgb2hsv(f_color0.rgb);
    hsv.x = mod(hsv.x + iIntensity, 1.0);
    f_color0.rgb = hsv2rgb(hsv);
}
