void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    // heart from shadertoy
    uv += vec2(-0.5, -0.7);
    uv *= 2. / iIntensity;
    float a = atan(uv.x, uv.y) / M_PI;
    float r = length(uv);
    float h = abs(a);
    float d = (13.0*h - 22.0*h*h + 10.0*h*h*h)/(6.0-5.0*h);

    vec4 c = vec4(1., 0.5, 0.5, 1. - smoothstep(-0.01, 0.01, r - d));
    gl_FragColor = composite(gl_FragColor, c);
}
