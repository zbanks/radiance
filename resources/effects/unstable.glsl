#property description Unstable second-order system

void main(void) {
    vec4 y1 = demultiply(texture(iChannel[0], uv));
    vec4 y2 = demultiply(texture(iChannel[1], uv));
    vec4 original = texture(iInput, uv);
    vec4 x0 = demultiply(original);

    vec4 y0;
    y0.rgb = 1.11 * y1.rgb - 9.0 * y2.rgb + 1.0 * x0.rgb;
    y0.a = x0.a;

    fragColor = mix(original, premultiply(y0), iIntensity);
}

#buffershader

void main(void) {
    // Delay 
    fragColor = texture(iChannel[0], uv);
}
