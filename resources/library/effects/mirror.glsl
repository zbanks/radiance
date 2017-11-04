#property description Place a vertical mirror to make the frame symmetric

void main(void) {
    float xPos = (1. - iIntensity) * 0.5 + 0.5;
    float x = -abs(uv.x - xPos) + xPos;
    fragColor = texture(iInput, vec2(x, uv.y));
}
