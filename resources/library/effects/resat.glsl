#property description Recolor output with perlin noise rainbow

void main(void) {
    float factor = pow(iIntensity, 0.6);
    vec3 noise_input = vec3(uv, iTime / 8.);
    float n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    //n = mod(n + 0.5, 1.0);
    //n = mod(n + hsl.r, 1.0);

    vec4 samp = texture(iInput, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g = 1.0 - (1.0 - hsl.g) * (1.0 - factor);
    //hsl.r = mix(hsl.r, n, iIntensity);
    hsl.r = mod(hsl.r + n * iIntensity, 1.0);
    fragColor.rgb = hsv2rgb(hsl);
    fragColor.a = samp.a;
}
