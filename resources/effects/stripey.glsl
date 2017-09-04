// Vertical stripes with a twinkle effect

void main(void) {
    float xv = round(uv.x * 20. * aspectCorrection.x); 
    gl_FragColor = texture2D(iChannel[0], uv);
    gl_FragColor.a *= exp(-iIntensity / 20.);

    if (rand(vec2(xv, iTime)) < exp(-iIntensity * 4.)) {
        gl_FragColor = texture2D(iInput, uv);
    }
}
