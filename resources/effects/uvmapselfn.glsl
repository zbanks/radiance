// Apply `uvmapself` repeatedly. Instant hellscape

#define N 5

void main(void) {
    // It looks better at low values of intensity
    float intensity = pow(iIntensity, 3.0);

    vec2 newUV = uv;
    for (int i = 0; i < N; i++) {
        vec4 map = texture2D(iInput, newUV);
        newUV = mix(newUV, map.rg, intensity * map.a);
    }
    gl_FragColor = texture2D(iInput, newUV);
}
