// Radiate color from the center based on audio
void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(c, fragColor);
}
#buffershader
void main(void) {

    fragColor = texture(iChannel[1], (uv - 0.5) * 0.98 + 0.5);
    fragColor.a *= exp((iIntensity - 2.) / 50.) * smoothstep(0, 0.01, length((uv - 0.5) * aspectCorrection));

    vec4 c = texture(iInput, uv);
    float s = smoothstep(0.90, 1., 1. - mod(iTime, 1.)) * iAudioLevel;
    c *=  min(3. * s, 1.);
    fragColor = composite(fragColor, c);
}
