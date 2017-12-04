#property description Zero order hold to the beat

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iInput, uv);

    float t = pow(2., round(6. * iIntensity - 4.));
    float a = 1.;

    if (iIntensity < 0.09)
        a = 0.;
    else if (mod(iTime, t) < 0.1)
        a = 0.;

    fragColor = mix(next, prev, a);
}
