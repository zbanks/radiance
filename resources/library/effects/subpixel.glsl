#property description Zoomin until you see subpixels

void main(void) {
    vec2 pt = uv - 0.5;
    float op = onePixel * 0.3;
    float factor = exp(-6. * iIntensity);

    vec2 coord = floor(pt * factor / op) * op;
    vec2 f = fract(pt * factor / op);
    vec4 c = texture(iInput, coord + 0.5);

    vec4 redSubpixel   = box(vec2(0.0, 0.) + f * vec2(3.6, 1.2)) * vec4(c.r, 0., 0., c.r);
    vec4 greenSubpixel = box(vec2(-1.2, 0.) + f * vec2(3.6, 1.2)) * vec4(0., c.g, 0., c.g);
    vec4 blueSubpixel  = box(vec2(-2.4, 0.) + f * vec2(3.6, 1.2)) * vec4(0., 0., c.b, c.b);

    vec4 c2 = redSubpixel + greenSubpixel + blueSubpixel;
    fragColor = texture(iInput, (uv - 0.5) * aspectCorrection * factor + 0.5);
    fragColor = mix(fragColor, c2, smoothstep(0.3, 0.6, iIntensity));
}
