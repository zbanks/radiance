// Apply smoothing over time with new hits happening instantly

void main(void) {
    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iFrame, uv);
    gl_FragColor.rgb = next.rgb;
    if (next.a > prev.a) {
        gl_FragColor = next;
    } else {
        prev.a *= pow(iIntensity, 0.1);
        gl_FragColor = composite(next, prev);
    }
    gl_FragColor.a = clamp(gl_FragColor.a, 0, 1);
    
}
