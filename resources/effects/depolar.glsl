// Convert rings to vertical lines

void main(void) {
    float angle  = uv.y * M_PI * 1.0;
    vec2 rtheta = uv.x * sqrt(2.) * vec2(sin(angle), -cos(angle));
    rtheta = (rtheta + 1.) / 2.;

    vec2 uv2 = mix(uv, rtheta, iIntensity);

    gl_FragColor = texture2D(iInput, uv2);
}
