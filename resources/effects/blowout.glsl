// Push colors towards extremes with smoothstep

void main(void) {
    vec4 c = demultiply(texture2D(iInput, uv));

    float f = mix(0.5, 0.05, iIntensity);
    vec3 d = smoothstep(0.5 - f, 0.5 + f, c.rgb);

    c.rgb = mix(c.rgb, d, iIntensity);
    gl_FragColor = premultiply(c);
}
