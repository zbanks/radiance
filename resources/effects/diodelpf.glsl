// Apply smoothing over time with new hits happening instantly

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iInput, uv);
    fragColor.rgb = next.rgb;
    if (next.a > prev.a) {
        fragColor = next;
    } else {
        prev *= pow(iIntensity, 0.1);
        fragColor = composite(next, prev);
    }
    fragColor.a = clamp(fragColor.a, 0, 1);
    
}
