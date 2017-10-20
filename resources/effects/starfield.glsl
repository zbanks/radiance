// Pixels radiating from the center

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}
#buffershader
void main(void) {
    fragColor = texture(iChannel[1], (uv - 0.5) * 0.99 + 0.5);
    fragColor.a *= exp(-1 / 20.);
    if (rand(vec3(uv, iTime)) < exp((iIntensity - 2.) * 4.))
        fragColor = vec4(1.);
}

#buffershader
void main(void) {
    fragColor = texture(iChannel[1], (uv - 0.5) * 0.99 + 0.5);
    fragColor *= exp(-1 / 20.);
    if (rand(vec3(uv, iTime)) < exp((iIntensity - 2.) * 4.))
        fragColor = vec4(1.);
}
