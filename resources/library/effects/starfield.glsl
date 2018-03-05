#property description Pixels radiating from the center

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}
#buffershader
void main(void) {
    float delta = iFrequency / 16.;
    vec2 uvSample = (uv - 0.5) * (1.0 - delta) + 0.5;
    fragColor = texture(iChannel[1], clamp(uvSample, 0., 1.));
    fragColor *= exp(-1. / 10.);
    float random = rand(texture(iNoise, uv) * iTime);
    if (random < exp((iIntensity - 2.) * 4.))
        fragColor = vec4(1.);
}
