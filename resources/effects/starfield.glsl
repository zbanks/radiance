// Pixels radiating from the center

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec4 c = texture2D(iChannel[1], uv);
    c.a *= smoothstep(0., 0.2, iIntensity);
    gl_FragColor = composite(gl_FragColor, c);
}
#buffershader
void main(void) {
    gl_FragColor = texture2D(iChannel[1], (uv - 0.5) * 0.99 + 0.5);
    gl_FragColor.a *= exp(-1 / 20.);
    if (rand(vec3(uv, iTime)) < exp((iIntensity - 2.) * 4.))
        gl_FragColor = vec4(1.);
}
