// Convert vertical lines to polygon rings

void main(void) {
    vec2 xy = gl_FragCoord.xy / iResolution;
    vec2 xy_cent = 2. * xy - 1.;
    float angle = abs(atan(xy_cent.y, xy_cent.x));
    float n_sides = (iIntensity * 5.) + 1.;
    float arc = 2. * M_PI / n_sides;
    float a1 = mod(angle, arc);
    float corr = cos(a1 - arc / 2.) / (sqrt(2.) * cos(arc / 2.));

    vec2 rtheta = vec2(length(xy_cent) * corr, 0.5 + angle / (2. * M_PI));
    vec2 uv = mix(xy, rtheta, clamp(iIntensity * 5., 0., 1.));

    gl_FragColor = texture2D(iFrame, uv);
}
