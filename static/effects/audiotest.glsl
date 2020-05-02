#property description Audio test pattern

void main(void) {
    vec4 c = vec4(iAudio.xyz, 1.0);
    fragColor = composite(texture(iInput, uv), c);
}

