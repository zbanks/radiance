#property description Green and blue base pattern

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec4 c = vec4(0., 0., 0., 1.);
    float ratio = 15.;
    c.r = 0.0;
    c.g = 0.5 * sin((normCoord.x + normCoord.y) * ratio) + 0.5;
    c.b = 0.5 * sin((normCoord.x - normCoord.y) * ratio) + 0.5;
    c.a = 1.0;

    fragColor = composite(texture(iInput, uv), c * iIntensity);
}
