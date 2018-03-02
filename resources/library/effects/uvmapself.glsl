#property description Apply `uvmap` using 1 input for both UV & RGB

void main(void) {
    vec4 map = texture(iInput, uv);
    vec2 newUV = mix(uv, map.rg, iIntensity * map.a * pow(defaultPulse, 2.));
    fragColor = texture(iInput, newUV);
}
