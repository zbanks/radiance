#property description Convert vertical lines to polygon rings

void main(void) {
    float n_sides = clamp(2.4 / (1. - iIntensity), 3., 10000.);
    float whole_sides = 0;
    n_sides = modf(n_sides, whole_sides);
    n_sides = whole_sides + smoothstep(0.2, 0.8, n_sides);

    vec2 xy_cent = 2. * uv - 1.;
    float angle = abs(atan(xy_cent.y, xy_cent.x));
    float arc = 2. * M_PI / n_sides;
    float a1 = mod(angle, arc);
    //float lengthFactor = sqrt(2.);
    float lengthFactor = 1.0;
    float corr = cos(a1 - arc / 2.) / (lengthFactor * cos(arc / 2.));

    vec2 rtheta = vec2(length(xy_cent) * corr, 0.5 + angle / (2. * M_PI));
    vec2 uv2 = mix(uv, rtheta, smoothstep(0., 0.2, iIntensity));

    fragColor = texture(iInput, uv2) * box(uv2);
}
