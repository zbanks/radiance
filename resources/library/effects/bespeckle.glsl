#property description Brownian-ish speckle effect

void main(void) {
    vec4 old = texture(iChannel[0], uv);
    vec4 new = texture(iInput, uv);

    float r = rand(vec3(uv, iTime));
    float k = pow(mix(1.0, r, iIntensity * defaultPulse), 16.0);
    fragColor = mix(old, new, k);

    // I don't think this is required, but just be safe
    fragColor.rgb = clamp(fragColor.rgb, 0.0, fragColor.a);
}
