#property description Smooth output, or first order (expontential) hold to the beat

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iChannel[1], uv);

    fragColor = mix(next, prev, pow(iIntensity, 0.4));
    fragColor = clamp(fragColor, 0., 1.);
}

#buffershader
void main(void) {
    vec4 transVec = texture(iChannel[2], vec2(0.));
    float trans = sign(transVec.r - transVec.g) * 0.5 + 0.5;
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
