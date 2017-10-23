// Zoom out

void main(void) {
    float factor = mix(1.0, 8.0, pow(iIntensity, 4.0));
    vec2 newUv = (uv - 0.5) * factor + 0.5;
    fragColor = texture(iInput, newUv);

    // If the coordinate is outside the box, make set the color to transparent
    vec2 s = step(vec2(0., 0.), newUv) - step(vec2(1., 1.), newUv);
    fragColor *= s.x * s.y;
}
