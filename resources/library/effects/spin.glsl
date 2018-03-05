#property description Spins the pattern round to the beat
#property frequency 1

void main(void) {
    float r;

    if(iFrequency > 0.) {
        r = mod(iTime * iFrequency, 1.0);
    } else {
        r = 0.; 
    }

    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    vec2 newUV = normCoord * rot / aspectCorrection;
    newUV *= min(iResolution.x, iResolution.y) / max(iResolution.x, iResolution.y);
    newUV += 0.5;

    vec4 oc = texture(iInput, uv);
    vec4 nc = texture(iInput, newUV);
    nc *= box(newUV);

    fragColor = mix(oc, nc, smoothstep(0., 0.5, iIntensity));
}
