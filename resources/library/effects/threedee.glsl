#property description Fake 3D-glasses effect

void main(void) {
    vec4 baseImage = texture(iInput, uv);
    float sep = iIntensity * ((baseImage.r + baseImage.g + baseImage.b) * 0.2 + 0.4) * 0.05;
    sep *= pow(defaultPulse, 0.5);

    vec4 redImage = texture(iInput, uv + vec2(sep, 0.));
    vec4 blueImage = texture(iInput, uv - vec2(sep, 0.));

    fragColor.r = mix(baseImage.r, redImage.r / redImage.a * baseImage.a, 0.9);
    fragColor.g = mix(baseImage.g, blueImage.g / blueImage.a * baseImage.a, 0.3);
    fragColor.b = mix(baseImage.b, blueImage.b / blueImage.a * baseImage.a, 0.6);
    fragColor.a = baseImage.a;
}
