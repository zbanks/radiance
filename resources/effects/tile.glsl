// Repeating tiles

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    float bins = pow(2, 4. * iIntensity);
    vec2 newUV = normCoord * bins;
    newUV = mod(newUV + 1.5, 2.);
    newUV = abs(newUV - 1.) - 0.5;
    newUV = newUV / aspectCorrection + 0.5;

    vec4 oc = texture2D(iInput, (uv - 0.5) * bins + 0.5);
    vec4 nc = texture2D(iInput, newUV);

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    nc.a *= smoothstep(0, 0.1, iIntensity);

    gl_FragColor = composite(oc, nc);
}
