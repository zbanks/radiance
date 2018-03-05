#property description Perlin noise green smoke

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec3 noise_input = vec3(normCoord * iIntensity * 4., iIntensity + iIntensityIntegral * iFrequency);
    float n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    n += (noise(8. * noise_input) - 0.5) * 0.125;
    n += (noise(16. * noise_input) - 0.5) * 0.0625;
    n = n / 3.;

    float a = clamp(n * n * 5., 0., 1.) * smoothstep(0., 0.2, iIntensity);

    fragColor = texture(iInput, uv);
    fragColor = composite(fragColor, vec4(0., a, 0., a));
}
