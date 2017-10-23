#property description Rolling shutter effect

void main(void) {
    float rate = 4.;
    float yv = 1.0 - mod(iTime, rate) / rate;
    vec4 old = texture(iChannel[0], uv);
    vec4 new = texture(iInput, uv);
    float dist = abs(uv.y - yv); // TODO: make this wrap around
    fragColor = mix(old, new, max(0., 1.0 - iFPS * 0.2 * dist));
    fragColor = mix(new, fragColor, pow(iIntensity, 0.1));
}
