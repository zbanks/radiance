void main(void) {
    gl_FragColor = texture2D(iChannel[1], (uv - 0.5) * 0.99 + 0.5);
    gl_FragColor.a *= exp(-1 / 20.);
    if (rand(vec3(uv, iTime)) < exp((iIntensity - 2.) * 4.))
        gl_FragColor = vec4(1.);
}
