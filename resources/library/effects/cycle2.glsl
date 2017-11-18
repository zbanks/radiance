#property description Switch between the two inputs on the beat
#property inputCount 2
void main() {
    vec4 l = texture(iInputs[0], uv);
    vec4 r = texture(iInputs[1], uv);

    float amt = mix(1., 8., iIntensity);
    float switcher = 0.5 * clamp(amt * sin(0.5 * iTime * M_PI), -1., 1.) + 0.5;

    float which = mix(0., switcher, smoothstep(0., 0.1, iIntensity));
    which = mix(which, 1., smoothstep(0.9, 1., iIntensity));

    fragColor = mix(l, r, which);
}
