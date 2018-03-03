#property description Replace green parts of the first input with the second
#property inputCount 2
void main() {
    vec4 m = texture(iInputs[0], uv);
    vec4 g = texture(iInputs[1], uv);

    fragColor = m;

    // x is 1.0 in pure green areas and ~0.0 elsewhere
    m = demultiply(m); // don't use alpha to detect green-ness
    float x = pow(clamp(m.g - (m.r + m.b) * 3.0, 0.0, 1.0), 0.2);
    x *= m.a; // Put alpha back in

    float parameter = iIntensity * pow(defaultPulse, 2.);
    fragColor = composite(fragColor, g * x * parameter);
}
