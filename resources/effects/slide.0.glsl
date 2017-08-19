// Slide the screen left-to-right

void main(void) {
    float deviation = iIntensityIntegral;
    vec2 uv2 = (uv - 0.5) * aspectCorrection;
    uv2.x = abs(mod(uv2.x + deviation + 1.5, 2.) - 1.) - 0.5;
    uv2  = uv2 / aspectCorrection + 0.5;

    vec4 oc = texture2D(iInput, uv);
    vec4 c = texture2D(iInput, uv2);

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    c.a *= smoothstep(0, 0.1, iIntensity);

    gl_FragColor = composite(oc, c);
}
