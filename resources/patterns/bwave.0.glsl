// Black sine wave from left to right.

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float x = (uv.x + uv.y) * 15 + iTime * 1;
    f_color0 = texture2D(iFrame, uv);
    f_color0.a *= 1.0 - iIntensity * (sin(x) / 2.0 + 0.5);
}
