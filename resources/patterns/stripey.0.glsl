// Vertical stripes with a twinkle effect

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float xv = round(uv.x * 20); 
    f_color0 = texture2D(iChannel[0], uv);
    f_color0.a *= exp(-iIntensity / 20.);

    if (rand(vec2(xv, iTime)) < exp(-iIntensity * 4.))
        f_color0 = texture2D(iFrame, uv);
}
