void main(void) {
    vec2 xy = gl_FragCoord.xy / iResolution;
    vec2 xy_cent = 2. * xy - 1.;

    vec2 uv = vec2(mix(xy.x, length(xy_cent) / sqrt(2.), iIntensity), mix(xy.y, abs(atan(xy_cent.x, -xy_cent.y) / M_PI), iIntensity));

    gl_FragColor = texture2D(iFrame, uv);
}
