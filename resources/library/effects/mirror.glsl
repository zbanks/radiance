#property description Place a vertical mirror to make the frame symmetric

void main(void) {
    float xPos = (1. - smoothstep(0., 0.2, iIntensity)) * 0.5 + 0.5;
    float x = -abs(uv.x - xPos) + xPos;
    x += iIntensity * 0.5 * pow(defaultPulse, 2.);
    fragColor = texture(iInput, vec2(x, uv.y));
}
