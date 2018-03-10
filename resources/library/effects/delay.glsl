#property description Introduce a delay (and reduce resolution)

#define SZ 6.    // Delay of SZ*SZ frames (36)
void main(void) {
    vec4 original = texture(iInput, uv);
    vec2 shift = floor(iResolution / SZ) / iResolution;
    vec2 uvNew = uv * shift;
    uvNew = mix(uvNew, uv, smoothstep(0.8, 0.9, iIntensity));
    vec4 delayed = texture(iChannel[1], uvNew);
    fragColor = mix(original, delayed, smoothstep(0., 0.2, iIntensity));
}

#buffershader

#define SZ 6.
void main(void) {
    vec2 shift = floor(iResolution / SZ) / iResolution;
    vec2 maxShift = shift * SZ;
    vec2 uvNext = uv + vec2(shift.x, 0.);
    if (uvNext.x > maxShift.x) {
        uvNext.x -= maxShift.x;
        uvNext.y += shift.y;
    }
    if (uvNext.y > maxShift.y) {
        uvNext.y -= maxShift.y;
        uvNext /= shift;
        fragColor = texture(iInput, uvNext);
    } else {
        fragColor = texture(iChannel[1], uvNext);
    }
}
