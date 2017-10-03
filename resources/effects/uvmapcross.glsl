// Use .rg as .uv and crossfade

#property inputCount 2
void main(void) {
    vec4 map = texture2D(iInputs[1], uv);
    vec2 newUV = mix(uv, map.rg, min(iIntensity * 2.0, 1.0) * map.a);
    vec4 mappedColor = texture2D(iInput, newUV);
    gl_FragColor = mix(mappedColor, map, max(0.0, iIntensity * 2.0 - 1.0));
}
