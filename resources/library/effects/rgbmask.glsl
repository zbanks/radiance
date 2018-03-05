#property description Use the R, G, B channels of the first input to mask the other 3 inputs
#property inputCount 4

void main() {
    vec4 m = texture(iInputs[0], uv);
    vec4 r = texture(iInputs[1], uv);
    vec4 g = texture(iInputs[2], uv);
    vec4 b = texture(iInputs[3], uv);

    float f = m.a * iIntensity * defaultPulse;
    fragColor.rgb = mix(m.rgb, vec3(0.0), f);
    fragColor.a = m.a;

    fragColor = composite(fragColor, r * m.r * f);
    fragColor = composite(fragColor, g * m.g * f);
    fragColor = composite(fragColor, b * m.b * f);
}
