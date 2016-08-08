void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float t = pow(2, round(6 * iIntensity - 4));
    float a = 0.98;
    if (iIntensity < 0.09 || mod(iTime, t) < 0.1)
        f_color0 = texture2D(iFrame, uv);
    else
        f_color0 = texture2D(iChannel[1], uv);
}
