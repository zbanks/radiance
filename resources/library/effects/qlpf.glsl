#property description Smooth output, or first order (expontential) hold to the beat

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iChannel[1], uv);

    float d = distance(prev, next) / 2.0;
    float k = pow(iIntensity, 0.3) * (1.0 - pow(d, mix(2.5, 1.0, iIntensity)));
    fragColor = mix(next, prev, k);
}

#buffershader
void main(void) {
    vec4 transVec = texture(iChannel[2], vec2(0.));
    float trans = step(0.5, transVec.r - transVec.g) + step(0., -iFrequency);
    float a = 1. - (1. - trans) * smoothstep(0., 0.2, iIntensity);
    fragColor = mix(texture(iChannel[1], uv), texture(iInput, uv), a);
}

#buffershader
// This shader stores the last value of defaultPulse in the red channel
// and the current value in the green channel.

void main(void) {
    float last = texture(iChannel[2], vec2(0.)).g;
    fragColor = vec4(last, mod(iFrequency * iTime, 1.), 0., 1.);
}
