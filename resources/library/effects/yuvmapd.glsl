#property description Use UV (from YUV) as delta-.uv and crossfade

#property inputCount 2
void main(void) {
    vec4 map = texture(iInputs[1], uv);
    vec3 yuv = rgb2yuv(demultiply(map).rgb);
    vec2 scaledUV = (yuv.gb - 0.5) * 4.0;
    vec2 newUV = uv + scaledUV * min(iIntensity * 2.0, 1.0) * map.a;
    newUV = clamp(newUV, 0., 1.);
    vec4 mappedColor = texture(iInput, newUV);
    fragColor = mix(mappedColor, map, max(0.0, iIntensity * 2.0 - 1.0));
}
