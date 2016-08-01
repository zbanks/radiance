// Basic white fill

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec4 c = vec4(1., 1., 1., iIntensity);
    gl_FragColor = composite(texture2D(iFrame, uv), c);
}
