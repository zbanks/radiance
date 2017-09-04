// Snowcrash: white static noise

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    float x = rand(vec3(gl_FragCoord.xy, iTime));
    vec4 c = vec4(x, x, x, 1.0);
    gl_FragColor = mix(gl_FragColor, c, iIntensity);
}
