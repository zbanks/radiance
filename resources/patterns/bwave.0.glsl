void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec4 c = vec4(0., 0., 0., 1.);
    float x = (uv.x + uv.y) * 15 + iTime * 1;
    c.a = iIntensity * (sin(x) / 2.0 + 0.5);

    gl_FragColor = composite(texture2D(iFrame, uv), c);
}
