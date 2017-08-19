void main() {
    vec4 m = texture2D(iInputs[0], uv);
    vec4 g = texture2D(iInputs[1], uv);

    gl_FragColor = m;

    // x is 1.0 in pure green areas and ~0.0 elsewhere
    m = demultiply(m); // don't use alpha to detect green-ness
    float x = pow(clamp(m.g - (m.r + m.b) * 3.0, 0.0, 1.0), 0.2);

    gl_FragColor = composite(gl_FragColor, g * x * iIntensity);
}
