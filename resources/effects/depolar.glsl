// Convert rings to vertical lines

void main(void) {
    float angle  = xy2.y * M_PI * 1.0;
    vec2 rtheta = xy2.x * sqrt(2.) * vec2(sin(angle), -cos(angle));
    rtheta = (rtheta + 1.) / 2.;

    vec2 uv2 = mix(uv, rtheta, iIntensity);

    fragColor = texture(iInput, uv2);
}
