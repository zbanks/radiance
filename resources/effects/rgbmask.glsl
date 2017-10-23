#property description Use the R, G, B channels of the first input to mask the other 3 inputs

void main() {
    vec4 m = texture(iInputs[0], uv);
    vec4 r = texture(iInputs[1], uv);
    vec4 g = texture(iInputs[2], uv);
    vec4 b = texture(iInputs[3], uv);

    fragColor = mix(m, vec4(0.0), iIntensity);
    m *= iIntensity;
    fragColor = composite(fragColor, r * m.r);
    fragColor = composite(fragColor, g * m.g);
    fragColor = composite(fragColor, b * m.b);
}
