#property inputCount 4

void main() {
    vec4 m = texture2D(iInputs[0], uv);
    vec4 r = texture2D(iInputs[1], uv);
    vec4 g = texture2D(iInputs[2], uv);
    vec4 b = texture2D(iInputs[3], uv);

    gl_FragColor = mix(m, vec4(0.0), iIntensity);
    m *= iIntensity;
    gl_FragColor = composite(gl_FragColor, r * m.r);
    gl_FragColor = composite(gl_FragColor, g * m.g);
    gl_FragColor = composite(gl_FragColor, b * m.b);
}
