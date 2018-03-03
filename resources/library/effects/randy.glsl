#property description Obnoxiously zoom and rotate ( in honor of  Raaaaandy Seidman )
#property frequency 0.5

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float t = iTime * iFrequency;

    float theta = 0.3 * (cos(t * 0.6) - sin(2. * t * 0.6)) * iIntensity;
    float zoom = 1. + 0.2 * (-1. + sin(t * 0.35) - cos(2. * t * 0.35)) * iIntensity;

    float s = sin(theta) / zoom;
    float c = cos(theta) / zoom;
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot;
    newUV = newUV / aspectCorrection + 0.5;

    vec4 nc = texture(iInput, newUV);
    nc *= box(newUV);

    fragColor = nc;
}
