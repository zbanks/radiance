// Fake edge detection based only on alpha

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    f_color0 = texture2D(iFrame, uv);
    f_color0.a = mix(f_color0.a, f_color0.a * (iIntensity - f_color0.a) / 0.25, iIntensity);
}
