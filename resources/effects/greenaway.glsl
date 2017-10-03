// Shift colors away from green (green is not a creative color)

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    float h = hsv.x;
    h = mod(h + 4. / 6., 1.0) - 3. / 6.;
    h *= (1 - iIntensity / 3.);
    h = mod(h - 1. / 6., 1.0);
    hsv.x = h;
    gl_FragColor.rgb = hsv2rgb(hsv);
}
