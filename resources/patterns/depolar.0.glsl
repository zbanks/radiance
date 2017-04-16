// Convert rings to vertical lines

void main(void) {
    vec2 xy = gl_FragCoord.xy / iResolution;
    float angle  = xy.y * M_PI * 2.0;
    vec2 rtheta = xy.x * sqrt(2.) * vec2(cos(angle), -sin(angle));
    rtheta = (rtheta + 1.) / 2.;

    vec2 uv = mix(xy, rtheta, iIntensity);

    gl_FragColor = texture2D(iFrame, uv);
}
