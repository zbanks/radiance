// Convert vertical lines to rings

void main(void) {
    vec2 xy_cent = 2. * v_uv - 1.;

    vec2 uv = vec2(mix(v_uv.x, length(xy_cent) / sqrt(2.), iIntensity), mix(v_uv.y, abs(atan(xy_cent.x, -xy_cent.y) / M_PI), iIntensity));

    f_color0 = texture2D(iFrame, uv);
}
