// Spatial edge detect filter (HPF)

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float d = 0.05;
    vec4 center = texture2D(iFrame, uv);
    vec4 left = texture2D(iFrame, uv - vec2(d, 0) / aspectCorrection);
    vec4 right = texture2D(iFrame, uv + vec2(d, 0) / aspectCorrection);
    vec4 up = texture2D(iFrame, uv + vec2(0, d) / aspectCorrection);
    vec4 down = texture2D(iFrame, uv - vec2(0, d) / aspectCorrection);
    left.rgb *= left.a;
    right.rgb *= right.a;
    up.rgb *= up.a;
    down.rgb *= down.a;
    vec4 outc = abs(left - right) + abs(up - down);
    gl_FragColor = clamp(outc * 1.5, 0, 1);
    gl_FragColor.a = center.a;
    gl_FragColor = mix(center, gl_FragColor, vec4(iIntensity));
}
