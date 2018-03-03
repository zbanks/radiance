#property description Digital glitching

void main(void) {
    vec4 c = texture(iInputs[0], uv);

    const int N_PARTITIONS = 3;
    const vec2 blockSizes[N_PARTITIONS] = vec2[](vec2(0.3, 0.5), vec2(1., 0.15), vec2(1., 1.));

    // Partition image into blocks a few different ways
    vec4 n1 = vec4(1.);
    vec4 n2 = vec4(1.);
    for (int i=0; i<N_PARTITIONS; i++) {
        vec2 block = floor(uv / blockSizes[i]) * blockSizes[i];
        vec4 t1 = texture(iNoise, vec2(iTime / 1000., (3.1 * block.x) + (7.8 * block.y)));
        n1 *= t1;
        vec4 t2 = texture(iNoise, vec2(iTime / 1000., (6.7 * block.x) + (2.9 * block.y)));
        n2 *= t2;
    }
    n1 = pow(n1, vec4(0.1));
    n2 = pow(n2, vec4(0.1));

    float parameter = iIntensity * (0.5 + 0.5 * pow(defaultPulse, 2.));

    // White noise glitch
    float noise_glitch = step(1. - 0.2 * parameter, n1.x);
    vec4 white_noise = texture(iNoise, uv + iTime);
    white_noise.rgb *= white_noise.a;
    c = mix(c, white_noise, noise_glitch);

    // Invert colors
    float invert_glitch = step(1. - 0.2 * parameter, n1.y);
    c.rgb = invert_glitch - 2. * (invert_glitch - 0.5) * c.rgb;

    // Solid color glitch
    float solid_glitch = step(1. - 0.2 * parameter, n1.z);
    c = mix(c, vec4(0., 1., 0., 1.), solid_glitch);

    // Shift glitch
    float shift_glitch = step(1. - 0.2 * parameter, n1.w);
    c = mix(c, texture(iInputs[0], uv - vec2(0.2, 0.)), shift_glitch);

    // Freeze glitch
    float freeze_glitch = step(1. - 0.2 * parameter, n2.x);
    c = mix(c, texture(iChannel[0], uv), freeze_glitch);

    fragColor = c;
}
