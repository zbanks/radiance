#property description White wave with hard edges

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    float xpos = iIntensityIntegral * 1.5;
    float xfreq = (iIntensity + 0.5) * 2.;
    float x = mod(normCoord.x * xfreq + xpos, 1.);
    fragColor = texture(iInput, uv);
    vec4 c = vec4(1., 1., 1., step(x, 0.3) * smoothstep(0., 0.5, iIntensity));
    fragColor = composite(fragColor, premultiply(c));
}
