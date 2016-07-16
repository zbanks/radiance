void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec3 noise_input = vec3((uv - 0.5) * iIntensity * 4, iIntensity + iIntensityIntegral * 0.1);
    float n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    n += (noise(8. * noise_input) - 0.5) * 0.125;
    n += (noise(16. * noise_input) - 0.5) * 0.0625;
    n = n / 3.;

    float a = clamp(n * n * 5., 0., 1.) * smoothstep(0., 0.2, iIntensity);

    gl_FragColor = vec4(0., 0.8, 0., a);
}
