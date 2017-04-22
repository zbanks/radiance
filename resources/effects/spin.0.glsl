// Rotate the screen

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float r = iIntensityIntegral * 0.6;
    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot / aspectCorrection + 0.5;

    vec4 oc = texture2D(iFrame, uv);
    vec4 nc = texture2D(iFrame, newUV);
    nc.a *= box(newUV);

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    nc.a *= smoothstep(0, 0.1, iIntensity);

    gl_FragColor = composite(oc, nc);
}
