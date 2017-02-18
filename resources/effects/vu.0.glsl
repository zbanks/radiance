// Blue vertical VU meter

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec3 audio = vec3(iAudioLow, iAudioMid, iAudioHi);

    audio = audio * 2. * iIntensity;

    vec3 draw = 1. - smoothstep(audio - onePixel, audio, vec3(abs(normCoord.x)));

    vec4 c = composite(composite(vec4(0., 0., 0.5, draw.x), vec4(0., 0., 1., draw.y)), vec4(0.3, 0.3, 1., draw.z));
    c = clamp(c, 0., 1.);
    gl_FragColor = composite(gl_FragColor, c);
}
