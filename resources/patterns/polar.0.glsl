// Convert vertical lines to rings

void main(void) {
    vec2 xy = gl_FragCoord.xy / iResolution;
    vec2 xy_cent = 2. * xy - 1.;
    float angle = atan(xy_cent.y, xy_cent.x);
    vec2 rtheta = vec2(length(xy_cent) / sqrt(2.), 0.5 + angle / (2. * M_PI));
    vec2 uv = mix(xy, rtheta, iIntensity);

    gl_FragColor = texture2D(iFrame, uv);
}
