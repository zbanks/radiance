#property description Dense diagonal white wave with hard edges

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    float xpos = iFrequency * iTime;
    float xfreq = (iIntensity + 0.2) * 30.;
    float x = mod((normCoord.x + normCoord.y) * xfreq + xpos, 1.);
    fragColor = texture(iInput, uv);
    vec4 c = vec4(1.) * step(x, 0.5) * smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}
