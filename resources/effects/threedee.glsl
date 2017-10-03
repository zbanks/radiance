// Fake 3D-glasses effect

void main(void) {
    vec4 baseImage = texture2D(iInput, uv);
    float sep = iIntensity * ((baseImage.r + baseImage.g + baseImage.b) * 0.2 + 0.4) * 0.1;

    vec4 redImage = texture2D(iInput, uv + vec2(sep, 0.));
    vec4 blueImage = texture2D(iInput, uv - vec2(sep, 0.));

    gl_FragColor.r = mix(baseImage.r, redImage.r / redImage.a * baseImage.a, 0.9);
    gl_FragColor.g = mix(baseImage.g, blueImage.g / blueImage.a * baseImage.a, 0.3);
    gl_FragColor.b = mix(baseImage.b, blueImage.b / blueImage.a * baseImage.a, 0.6);
    gl_FragColor.a = baseImage.a;
}
