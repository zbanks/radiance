// Pink heart

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);

    // heart from shadertoy
    vec2 uv = v_uv + vec2(-0.5, -0.55);
    uv *= 2. / iIntensity;
    float a = atan(uv.x, uv.y) / M_PI;
    float r = length(uv);
    float h = abs(a);
    float d = (13.0*h - 22.0*h*h + 10.0*h*h*h)/(6.0-5.0*h);

    vec4 c = vec4(1., 0.5, 0.5, 1. - smoothstep(-0.01, 0.01, r - d));
    f_color0 = composite(f_color0, c);
}
