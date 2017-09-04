// Spins the pattern round to the beat

void main(void) {
    float r;
    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.25) freq = 32.;
    else if(iIntensity < 0.35) freq = 16.;
    else if(iIntensity < 0.45) freq = 8.;
    else if(iIntensity < 0.55) freq = 4.;
    else if(iIntensity < 0.65) freq = 2.;
    else if(iIntensity < 0.75) freq = 1.;
    else if(iIntensity < 0.85) freq = 0.5;
    else freq = 0.25;

    if(freq > 0) {
        r = mod(iTime, freq) / freq;
    } else {
        r = 0; 
    }

    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot / aspectCorrection;
    newUV *= min(iResolution.x, iResolution.y) / max(iResolution.x, iResolution.y);
    newUV += 0.5;

    vec4 oc = texture2D(iInput, uv);
    vec4 nc = texture2D(iInput, newUV);
    nc.a *= box(newUV);

    gl_FragColor = mix(oc, nc, smoothstep(0, 0.2, iIntensity));
}
