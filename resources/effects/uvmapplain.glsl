#property description Use .rg as .uv without crossfading

#property inputCount 2
void main(void) {
    vec4 map = texture(iInputs[1], uv);
    vec2 newUV = mix(uv, map.rg, iIntensity * map.a);
    fragColor = texture(iInput, newUV);
}
