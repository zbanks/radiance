#property description Strange tentacles

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    // Build a vector to use to sample perlin noise from
    // .xy ~ coordinate, .z ~ time + audio, .w ~ fixed integer per call to noise()
    vec4 noise_input = vec4(0);
    noise_input.xy = 5 * normCoord * smoothstep(0., 0.4, iIntensity);
    noise_input.z = iIntensityIntegral * 0.4 + iAudioLevel * mix(0.2, 0.7, iIntensity);

    float n;
    // Sample perlin noise with many octaves
    n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    n += (noise(8. * noise_input) - 0.5) * 0.125;
    n += (noise(16. * noise_input) - 0.5) * 0.0625;
    // Take the noise and create "ridges", call it `a`
    n -= 0.5;
    n *= mix(14.0, 4.0, iAudioLevel);
    n = clamp(abs(n), 0., 1.);
    n = 1.0 - pow(n, mix(1.0, 4.0, iAudioLevel));
    float a = n;

    // Sample perlin noise with just a few octaves, we'll use it for color
    noise_input.w = 1337.;
    n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    n = clamp(n, 0., 1.);
    n = pow(n, 2.0);
    float b = n;

    // Sample perlin noise for distortion
    noise_input.w = 9876.;
    n = noise(noise_input) - 0.1;
    n += (noise(2. * noise_input) - 0.5) * 0.5;
    n += (noise(4. * noise_input) - 0.5) * 0.25;
    n += (noise(8. * noise_input) - 0.5) * 0.125;

    // Mix the new noise with `a` to create fringing around the tentacles
    float c = n * mix(0.08, 0.4, a);
    // Distort (re-use `b` as angle)
    vec2 uvNew = uv;
    uvNew.x += cos(b) * c;
    uvNew.y += sin(b) * c;

    // Make a black-ish/blue-ish color
    vec4 color = vec4(0.05, 0.4, 0.5, 1.);
    color.rgb *= b;
    color.rg *= mix(0.7, 1.0, c);
    color *= a * smoothstep(0., 0.2, iIntensity);

    vec4 under = texture(iInput, mix(uv, uvNew, iIntensity));
    fragColor = composite(under, color);
}
