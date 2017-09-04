// Applies vignette

float hyper_length(vec2 c, float f) {
    return pow(abs(pow(c.x, f)) + abs(pow(c.y, f)), 1. / f);
}

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec2 coord = (uv - 0.5);

    float f = 3. / iIntensity;
    float edge1 = 2 * hyper_length(coord, f);
    float edge2 = 0.5 * length(coord / max(abs(coord.x), abs(coord.y)));

    gl_FragColor *= 1. - smoothstep(1. - 0.5 * iIntensity, 1., edge1);
}
