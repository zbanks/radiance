// White slit for testing

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    //float x = ((iIntensity / 0.8 + 0.2) - 0.5) * 2.0;
    float x = (iIntensity - 0.2) / 0.8;
    vec4 color = vec4(1.0, 1.0, 1.0, smoothstep(0., 0.2, iIntensity));
    gl_FragColor = texture2D(iFrame, uv);
    if (abs(x - uv.x) < 0.02) {
        gl_FragColor = composite(gl_FragColor, color);
    }
}
