#property description Fix out-of-bounds values in premultiplied-alpha space

void main(void) {
    vec4 c = texture(iInput, uv);
    float a = max(max(c.r,c.g),max(c.b,c.a));
    fragColor = vec4(c.rgb,mix(c.a,a,iIntensity * defaultPulse));
}
