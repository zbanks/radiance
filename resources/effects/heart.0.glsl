// Pink heart

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);

    // heart from shadertoy
    vec2 normCoord = (uv - 0.5) * aspectCorrection + vec2(0., -0.15);
    normCoord *= 2. / iIntensity;
    float a = atan(normCoord.x, normCoord.y) / M_PI;
    float r = length(normCoord);
    float h = abs(a);
    float d = (13.0*h - 22.0*h*h + 10.0*h*h*h)/(6.0-5.0*h);

    vec4 c = vec4(1., 0.5, 0.5, 1. - smoothstep(-0.01, 0.01, r - d));
    gl_FragColor = composite(gl_FragColor, c);
}
