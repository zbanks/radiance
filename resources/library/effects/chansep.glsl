#property description Red / green / blue color channel separation
#property frequency 1
// AKA "Chromatic Aberration"

void main(void) {
    float spin = iTime * 0.2;
    float separate = iIntensity * 0.1 * cos(iTime * M_PI * 0.25 * iFrequency);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2 redOffset = normCoord - separate * vec2(cos(spin), sin(spin));
    vec2 greenOffset = normCoord - separate * vec2(cos(2. + spin), sin(2. + spin));
    vec2 blueOffset = normCoord - separate * vec2(cos(4. + spin), sin(4. + spin));

    vec4 redImage = texture(iInput, redOffset / aspectCorrection + 0.5);
    vec4 greenImage = texture(iInput, greenOffset / aspectCorrection + 0.5);
    vec4 blueImage = texture(iInput, blueOffset / aspectCorrection + 0.5);

    vec3 rgb = vec3(redImage.r, greenImage.g, blueImage.b);
    float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    fragColor = vec4(rgb, a_out);
}
