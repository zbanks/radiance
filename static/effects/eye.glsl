#property description Distort effect that makes the image look more like an eye

void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;

    //float lengthFactor = sqrt(2.);
    float lengthFactor = 1.0;

    float len = length(normCoord) / lengthFactor;
    float angle = abs(atan(normCoord.x, normCoord.y) / M_PI);

    vec2 newUVCenter = vec2(angle, 2. * len) - 0.5;
    vec2 newUVMiddle = vec2(2. * len - 1., angle) - 0.5;
    vec2 newUVOutside = 0.5 * normCoord;

    float a = iIntensity * 0.2; // Make the eye open a little
    float centerShape = smoothstep(0.1 + a, 0.2 + a, len);
    float eyeShape = smoothstep(0.9 + a, 1.0 + a, length(abs(normCoord) + vec2(0., 0.7)));

    vec2 newUV = mix(newUVCenter, newUVMiddle, centerShape);
    newUV = mix(newUV, newUVOutside, eyeShape);

    newUV = newUV / aspectCorrection + 0.5;

    fragColor = texture(iInput, mix(uv, newUV, iIntensity));
}
