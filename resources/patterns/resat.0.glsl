// Recolor output with noise rainbow

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float factor = pow(iIntensity, 0.6);
    vec3 noise_input = vec3(uv, iTime / 8.);
    float n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;

    vec4 samp = texture2D(iFrame, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g = 1.0 - (1.0 - hsl.g) * (1.0 - factor);
    hsl.r = mix(hsl.r, n, iIntensity);
    gl_FragColor.rgb = hsv2rgb(hsl);
    gl_FragColor.a = samp.a;
}
