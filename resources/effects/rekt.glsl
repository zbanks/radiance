#property description Get (covered in random) rekt(angles)

void main(void) {
    vec4 under = texture(iInput, uv);

    vec4 over = texture(iChannel[1], uv);
    over.a = step(0.001, over.a);
    over.a *= smoothstep(0., 0.2, iIntensity);
    over.rgb *= over.a;

    fragColor = composite(under, over);
}

#buffershader

void main(void) {
    vec4 before = texture(iChannel[1], uv);
    before.a = max(before.a - 0.01, 0.);

    vec2 xy = vec2(rand(vec2(iTime, 0.)), rand(vec2(iTime, 1.)));
    vec2 wh = vec2(rand(vec2(iTime, 2.)), rand(vec2(iTime, 3.))) * 0.3 + 0.1;
    vec4 color = vec4(rand(vec2(iTime, 4.)), rand(vec2(iTime, 5.)), rand(vec2(iTime, 6.)), 1.0);

    float inside = box((uv - xy) / wh + 0.5);
    fragColor = mix(before, color, inside);
}
