#property description Convert rings to vertical lines

void main(void) {
    float angle  = uv.y * M_PI * 1.0;
    //float lengthFactor = sqrt(2.);
    float lengthFactor = 1.0;
    vec2 rtheta = uv.x * lengthFactor * vec2(sin(angle), -cos(angle));
    rtheta = (rtheta + 1.) / 2.;

    vec2 uv2 = mix(uv, rtheta, iIntensity);

    fragColor = texture(iInput, uv2);
}
