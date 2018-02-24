#property description Brownian-ish speckle effect with perlin noise

void main(void) {
    vec4 old = texture(iChannel[0], uv);
    vec4 new = texture(iInput, uv);

    float r = noise(vec3(uv * 16., iTime * iFrequency));
    float k = pow(mix(1.0, r, iIntensity), 2.0);
    fragColor = mix(old, new, k);

    // I don't think this is required, but just be safe
    fragColor.rgb = clamp(fragColor.rgb, 0.0, fragColor.a);
}
