#property description Stick chunks to the screen
#propert frequency 1

void main(void) {
    vec4 hold = texture(iChannel[1], uv);
    vec4 inp = texture(iInput, uv);
    hold.a = inp.a;
    hold = premultiply(hold);
    fragColor = mix(inp, hold, iIntensity);
}

#buffershader

void main(void) {
    vec4 inp = texture(iInput, uv);
    inp = premultiply(inp);
    vec4 hold = texture(iChannel[1], uv);

    float k = hold.a;
    float d = distance(inp.rgb, hold.rgb) / sqrt(3.);

    k += pow(d, 0.5) * 0.3 - 0.03;
    k *= pow(iIntensity, 0.3);
    k += pow(defaultPulse, 0.5); // I don't really get whats going on here
    k = clamp(k, 0., 1.);

    fragColor.rgb = mix(inp.rgb, hold.rgb, k);
    fragColor.a = k;
}
