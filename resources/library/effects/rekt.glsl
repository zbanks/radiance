#property description Get (covered in random) rekt(angles)
#property frequency 2

void main(void) {
    vec4 under = texture(iInput, uv);

    vec4 over = texture(iChannel[1], uv);
    over.a = step(1. - iIntensity + 0.005, over.a);
    //over.a *= smoothstep(0., 0.2, iIntensity);
    over.rgb *= over.a;

    fragColor = composite(under, over);
}

#buffershader

void main(void) {
    vec4 before = texture(iChannel[1], uv);
    before.a = max(before.a - 0.005, 0.);

    vec4 transVec = texture(iChannel[2], vec2(0.));
    float trans = step(0.5, transVec.r - transVec.g) + step(0., -iFrequency);

    vec2 xy = vec2(rand(vec2(iTime, 0.)), rand(vec2(iTime, 1.)));
    vec2 wh = vec2(rand(vec2(iTime, 2.)), rand(vec2(iTime, 3.))) * 0.3 + 0.1;
    vec4 color = vec4(rand(vec2(iTime, 4.)), rand(vec2(iTime, 5.)), rand(vec2(iTime, 6.)), 1.0);

    float inside = box((uv - xy) / wh + 0.5) * trans;
    fragColor = mix(before, color, inside);
}

#buffershader
// This shader stores the last value of defaultPulse in the red channel
// and the current value in the green channel.

void main(void) {
    float last = texture(iChannel[2], vec2(0.)).g;
    fragColor = vec4(last, mod(iFrequency * iTime, 1.), 0., 1.);
}
