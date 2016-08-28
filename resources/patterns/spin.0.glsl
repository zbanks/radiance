// Spins the pattern round to the beat

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    float r;
    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.15) freq = 16.;
    else if(iIntensity < 0.25) freq = 8.;
    else if(iIntensity < 0.35) freq = 4.;
    else if(iIntensity < 0.45) freq = 2;
    else if(iIntensity < 0.55) freq = 1;
    else if(iIntensity < 0.65) freq = 0.5;
    else if(iIntensity < 0.75) freq = 0.25;
    else freq = 0.125;

    if(freq > 0) {
        r = mod(iTime, freq) / freq;
    } else {
	r = 0; 
    }

    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * rot + 0.5);
}
