#property description Convert vertical lines to rings

void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;

    //float lengthFactor = sqrt(2.);
    float lengthFactor = 1.0;

    vec2 newUV = vec2(length(normCoord) / lengthFactor, abs(atan(normCoord.x, -normCoord.y) / M_PI)) - 0.5;
    newUV = newUV / aspectCorrection + 0.5;

    fragColor = texture(iInput, mix(uv, newUV, iIntensity));
}
