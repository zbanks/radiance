#property description Base identity pattern for use with `uvmap`

void main(void) {
    vec4 base = texture(iInput, uv);
    // The .b channel could be anything; 0.0 plays well with `rainbow`
    vec4 c = vec4(uv, 0.0, 1.0);
    fragColor = mix(base, c, iIntensity);
}
