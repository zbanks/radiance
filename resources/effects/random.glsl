// Snowcrash: white static noise

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    float x = rand(vec4(gl_FragCoord.xy, iTime, 1.));
    float y = rand(vec4(gl_FragCoord.xy, iTime, 2.));
    float z = rand(vec4(gl_FragCoord.xy, iTime, 3.));
    vec4 c = vec4(x, y, z, 1.0);
    gl_FragColor = mix(gl_FragColor, c, iIntensity);
}
