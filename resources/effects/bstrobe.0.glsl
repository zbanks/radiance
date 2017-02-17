// Full black strobe. Intensity increases frequency

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.15) freq = 4.;
    else if(iIntensity < 0.25) freq = 2.;
    else if(iIntensity < 0.35) freq = 1.;
    else if(iIntensity < 0.45) freq = 0.5;
    else if(iIntensity < 0.55) freq = 0.25;
    else if(iIntensity < 0.65) freq = 0.125;
    else if(iIntensity < 0.75) freq = 0.0625;
    else freq = 0.03125;

    if(freq > 0) {
        c = vec4(0., 0., 0., 1. - mod(iTime, freq) / freq);
        //c.a *= pow(iIntensity, 0.3);
        gl_FragColor = composite(gl_FragColor, c);
    }
}
