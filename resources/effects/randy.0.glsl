// Obnoxiously zoom and rotate

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float theta = 0.3 * (sin(iIntensityIntegral * 0.6) - sin(2 * iIntensityIntegral * 0.6)) * iIntensity;
    float zoom = 1. + 0.2 * (-1. + sin(iIntensityIntegral * 0.35) - sin(2 * iIntensityIntegral * 0.35)) * iIntensity;

    float s = sin(theta) / zoom;
    float c = cos(theta) / zoom;
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot;
    newUV = newUV / aspectCorrection + 0.5;

    vec4 nc = texture2D(iInput, newUV);
    nc *= box(newUV);

    gl_FragColor = nc;
}
