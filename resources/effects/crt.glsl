// CRT-style effect

void main(void) {
    vec2 separate = iIntensity * vec2(0.005, 0.0);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2 redOffset = normCoord - separate;
    vec2 greenOffset = normCoord;
    vec2 blueOffset = normCoord + separate;

    vec4 redImage = texture(iInput, redOffset / aspectCorrection + 0.5);
    vec4 greenImage = texture(iInput, greenOffset / aspectCorrection + 0.5);
    vec4 blueImage = texture(iInput, blueOffset / aspectCorrection + 0.5);

    vec3 rgb = vec3(redImage.r, greenImage.g, blueImage.b);
    rgb *= mix(1.0, 1.0 - pow(abs(sin(greenOffset.y * 160.0)), 16.), iIntensity / 3.0);
    rgb.r *= mix(1.0, 1.0 - pow(abs(sin(redOffset.x * 200.0)), 6.), iIntensity);
    rgb.g *= mix(1.0, 1.0 - pow(abs(sin(greenOffset.x * 200.0)), 6.), iIntensity);
    rgb.b *= mix(1.0, 1.0 - pow(abs(sin(blueOffset.x * 200.0)), 6.), iIntensity);
    fragColor = vec4(rgb, greenImage.a);
}
