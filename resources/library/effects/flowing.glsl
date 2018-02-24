#property description Sort of like rainbow but for lightness, avoiding the edges
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);

    float deviation = mod(iTime * iFrequency * 0.25, 1.);
    float spatialFrequency = 8.;
    float newLightness = hsv.z * mod(spatialFrequency * hsv.z + deviation, 1.);

    float amount = iIntensity;

    // Avoid the edges to smooth the discontinuity
    amount *= smoothstep(0., 0.4, newLightness);
    amount *= 1. - smoothstep(0.6, 1., newLightness);

    hsv.z = mix(hsv.z, newLightness, amount);
    fragColor.rgb = hsv2rgb(hsv);
}
