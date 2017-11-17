#property description Rotate the screen

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float r = iIntensityIntegral * 0.6;
    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot / aspectCorrection;
    //newUV *= min(iResolution.x, iResolution.y) / max(iResolution.x, iResolution.y) * sqrt(0.5);
    newUV += 0.5;

    vec4 oc = texture(iInput, uv);
    vec4 nc = texture(iInput, newUV) * box(newUV);

    fragColor = mix(oc, nc, smoothstep(0, 0.2, iIntensity));
}
