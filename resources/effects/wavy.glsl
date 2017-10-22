// Distort the screen

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2 shift = vec2(0.);
    shift += cos(M_PI * normCoord) * sin(iTime * vec2(0.1, 0.13));
    shift += cos(M_PI * normCoord * 2.) * sin(iTime * vec2(0.33, -0.23)) / 2.;
    shift += cos(M_PI * normCoord * 3.) * sin(iTime * vec2(0.35, -0.53)) / 3.;
    shift += cos(M_PI * normCoord * 4.) * sin(iTime * vec2(-0.63, -0.20)) / 4.;
    shift += cos(M_PI * normCoord * 5.) * sin(iTime * vec2(-0.73, 0.44)) / 5.;
    shift += cos(M_PI * normCoord * 6.) * sin(iTime * vec2(-0.73, 0.74)) / 6.;
    shift += cos(M_PI * normCoord * 7.) * sin(iTime * vec2(-1.05, -0.52)) / 7.;
    shift += cos(M_PI * normCoord * 8.) * sin(iTime * vec2(1.45, -1.22)) / 8.;

    shift += sin(M_PI * normCoord * 5.) * sin(iTime * vec2(0.79, -0.47)) / 5.;
    shift += sin(M_PI * normCoord * 6.) * sin(iTime * vec2(0.33, 0.79)) / 6.;
    shift += sin(M_PI * normCoord * 7.) * sin(iTime * vec2(1.15, -0.53)) / 7.;
    shift += sin(M_PI * normCoord * 8.) * sin(iTime * vec2(-1.36, -1.12)) / 8.;

    float amount = 0.1 * iIntensity;

    fragColor = texture(iInput, (normCoord + shift * amount) / aspectCorrection + 0.5);
}
