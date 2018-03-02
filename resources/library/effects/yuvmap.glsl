#property description Use UV (from YUV) as .uv

#property inputCount 2
void main(void) {
    vec4 map = texture(iInputs[1], uv);
    vec3 yuv = rgb2yuv(demultiply(map).rgb);
    vec2 scaledUV = (yuv.gb - 0.5) * 4.0 + 0.5;
    vec2 newUV = mix(uv, scaledUV, iIntensity * map.a * pow(defaultPulse, 2.));
    vec4 mappedColor = texture(iInput, newUV);
    fragColor = mappedColor;
}
