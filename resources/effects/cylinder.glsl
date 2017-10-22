// Wrap the parent texture on a spinning cylinder

void main(void) {
    float angle = iIntensityIntegral * 32.0;
    angle += 2 * asin(2 * (uv.x - 0.5));

    float x = mod(angle / (M_PI), 2.0);
    x -= 1.0;
    x = abs(x);

    vec2 new_uv = vec2(x, uv.y);
    new_uv = mix(uv, new_uv, smoothstep(0., 0.15, iIntensity));
    fragColor = texture(iInput, new_uv);
}
