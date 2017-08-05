// Red / green / blue color channel separation
// AKA "Chromatic Aberration"

void main(void) {
    float spin = iTime * 0.2;
    float separate = iIntensity * 0.1 * cos(iTime * M_PI * 0.25);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2 redOffset = normCoord - separate * vec2(cos(spin), sin(spin));
    vec2 greenOffset = normCoord - separate * vec2(cos(2. + spin), sin(2. + spin));
    vec2 blueOffset = normCoord - separate * vec2(cos(4. + spin), sin(4. + spin));

    vec4 redImage = texture2D(iFrame, redOffset / aspectCorrection + 0.5);
    vec4 greenImage = texture2D(iFrame, greenOffset / aspectCorrection + 0.5);
    vec4 blueImage = texture2D(iFrame, blueOffset / aspectCorrection + 0.5);

    vec3 rgb = vec3(redImage.r * redImage.a, greenImage.g * greenImage.a, blueImage.b * blueImage.a);
    float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    gl_FragColor = vec4(rgb / a_out, a_out);
}
