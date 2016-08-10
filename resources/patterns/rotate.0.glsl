// Rotate the screen

void main(void) {
    vec2 uv = gl_FragCoord.xy / v_size;

    float r = iIntensity;
    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    f_color0 = texture2D(iFrame, (uv - 0.5) * rot + 0.5);
}
