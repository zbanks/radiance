#property description Basic white fill

void main(void) {
    vec4 c = vec4(1.) * iIntensity;
    fragColor = composite(texture(iInput, uv), c);
}
