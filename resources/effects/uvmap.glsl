// Use .rg as .uv

void main(void) {
    vec4 map = texture2D(iInputs[1], uv);
    vec2 newUV = mix(uv, map.rg, iIntensity);
    gl_FragColor = texture2D(iInput, newUV);
}
