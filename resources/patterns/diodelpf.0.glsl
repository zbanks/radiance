// Apply smoothing over time with new hits happening instantly

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iFrame, uv);
    f_color0.rgb = next.rgb;
    if (next.a > prev.a) {
        f_color0 = next;
    } else {
        prev.a *= pow(iIntensity, 0.1);
        f_color0 = composite(next, prev);
    }
    f_color0.a = clamp(f_color0.a, 0, 1);
    
}
