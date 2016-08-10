// Spatial edge detect filter (HPF)

void main(void) {
    vec2 uv = gl_FragCoord.xy / v_size;
    float d = 0.05;
    vec4 center = texture2D(iFrame, uv);
    vec4 left = texture2D(iFrame, uv - vec2(d, 0));
    vec4 right = texture2D(iFrame, uv + vec2(d, 0));
    vec4 up = texture2D(iFrame, uv + vec2(0, d));
    vec4 down = texture2D(iFrame, uv - vec2(0, d));
    left.rgb *= left.a;
    right.rgb *= right.a;
    up.rgb *= up.a;
    down.rgb *= down.a;
    vec4 outc = abs(left - right) + abs(up - down);
    f_color0 = clamp(outc * 1.5, 0, 1);
    f_color0.a = center.a;
    f_color0 = mix(center, f_color0, vec4(iIntensity));
}
