#property description Use .rg as .uv without crossfading

#property inputCount 2
void main(void) {
    vec4 map = texture(iInputs[1], uv);
    vec2 newUV = mix(uv, map.rg, iIntensity * map.a * pow(defaultPulse, 2.));
    fragColor = texture(iInput, newUV);
}
