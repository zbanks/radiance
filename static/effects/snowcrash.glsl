#property description Snowcrash: white static noise

void main(void) {
    fragColor = texture(iInput, uv);
    float x = rand(vec3(gl_FragCoord.xy, iTime));
    vec4 c = vec4(x, x, x, 1.0);
    fragColor = mix(fragColor, c, iIntensity);
}
