// Radiate color from the center based on audio
void main(void) {

    gl_FragColor = texture2D(iInput, uv);
    vec4 c = texture2D(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    gl_FragColor = composite(c, gl_FragColor);
}
#buffershader
void main(void) {

    gl_FragColor = texture2D(iChannel[1], (uv - 0.5) * 0.98 + 0.5);
    gl_FragColor *= exp((iIntensity - 2.) / 50.) * smoothstep(0, 0.01, length((uv - 0.5) * aspectCorrection));

    vec4 c = texture2D(iInput, uv);
    float s = smoothstep(0.90, 1., 1. - mod(iTime, 1.)) * mix(0.01, 1.0, iAudioLevel);
    c *=  min(3. * s, 1.);
    gl_FragColor = composite(gl_FragColor, c);
}
