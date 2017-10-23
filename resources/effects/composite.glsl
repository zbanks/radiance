#property description Overlay the second input on top of the first
#property inputCount 2
void main() {
    vec4 l = texture(iInputs[0], uv);
    vec4 r = texture(iInputs[1], uv);
    fragColor = composite(l, r * iIntensity);
}
