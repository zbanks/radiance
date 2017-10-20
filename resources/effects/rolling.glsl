// Rolling shutter effect

void main(void) {
    float rate = 4.;
    float yv = 1.0 - mod(iTime, rate) / rate;
    vec4 old = texture2D(iChannel[0], uv);
    vec4 new = texture2D(iInput, uv);
    float dist = abs(uv.y - yv); // TODO: make this wrap around
    gl_FragColor = mix(old, new, max(0., 1.0 - iFPS * 0.2 * dist));
    gl_FragColor = mix(new, gl_FragColor, pow(iIntensity, 0.1));
}
