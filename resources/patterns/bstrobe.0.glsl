// Full black strobe. Intensity increases frequency

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);
    vec4 c;

    if(iIntensity >= 0.05){
        float freq = 4 * exp2(-floor((iIntensity - 0.05)*10));
        c = vec4(0., 0., 0., 1. - mod(iTime, freq) / freq);
        //c.a *= pow(iIntensity, 0.3);
        f_color0 = composite(f_color0, c);
    }
}
