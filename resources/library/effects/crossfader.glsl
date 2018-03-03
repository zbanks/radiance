#property description Mix between the two inputs
#property inputCount 2
void main() {
    vec4 l = texture(iInputs[0], uv);
    vec4 r = texture(iInputs[1], uv);
    fragColor = mix(l, r, iIntensity * pow(defaultPulse, 2.));
}
