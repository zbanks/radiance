#property description Vertical stripes with a twinkle effect
#property frequency 1

void main(void) {
    float xv = round(uv.x * 20. * aspectCorrection.x); 
    fragColor = texture(iChannel[0], uv);
    fragColor *= exp(-iIntensity * iFrequency / 20.);

    if (rand(vec2(xv, iTime)) < exp(-iIntensity * 4.)) {
        fragColor = texture(iInput, uv);
    }
}
