void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float x = (uv.x + uv.y) * 15 + iTime * 1;
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor.a *= iIntensity * (sin(x) / 2.0 + 0.5);
}
