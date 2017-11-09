#property description Introduce a delay (and make things blurry)

#define SZ 6    // Delay of SZ*SZ frames (36)
void main(void) {
    vec4 original = texture(iInput, uv);
    vec4 delayed = texture(iChannel[1], uv / SZ);
    fragColor = mix(original, delayed, smoothstep(0., 0.2, iIntensity));
}

#buffershader

#define SZ 6
void main(void) {
    vec2 uvNext = uv + vec2(1., 0.) / SZ;
    float i;
    uvNext.x = modf(uvNext.x, i);
    uvNext.y += i / SZ;
    if (uvNext.y > 1.0) {
        fragColor = texture(iInput, 1.0 - (1.0 - uv) * SZ);
    } else {
        fragColor = texture(iChannel[1], uvNext);
    }
}
