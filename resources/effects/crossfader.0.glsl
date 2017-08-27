void main() {
    vec4 l = texture2D(iInputs[0], uv);
    vec4 r = texture2D(iInputs[1], uv);
    gl_FragColor = mix(l, r, iIntensity);
}
