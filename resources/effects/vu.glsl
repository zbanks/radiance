// Blue vertical VU meter

void main(void) {
    fragColor = texture(iInput, uv);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec3 audio = vec3(iAudioLow, iAudioMid, iAudioHi);

    audio = audio * 2. * iIntensity;

    vec3 draw = 1. - smoothstep(audio - onePixel, audio, vec3(abs(normCoord.x)));
    vec4 dLow = vec4(0.0, 0.0, 0.5, 1.0) * draw.x;
    vec4 dMid = vec4(0.0, 0.0, 1.0, 1.0) * draw.y;
    vec4 dHi  = vec4(0.3, 0.3, 1.0, 1.0) * draw.z;

    vec4 c = composite(composite(dLow, dMid), dHi);
    c = clamp(c, 0., 1.);
    fragColor = composite(fragColor, c);
}
