#property description Cycle through different UV map coordinates
#property inputCount 2

float bound(float low, float high, float eps, float t) {
    // Smooth 'box'-like function; returns 1.0 if t in [low, high] and 0.0 if outside
    // eps is a smoothing factor
    return smoothstep(low - eps, low + eps, t) - smoothstep(high - eps, high + eps, t);
}

void main(void) {
    vec4 map = texture(iInputs[1], uv);

    // Cycle through 4 states based on beat counter, slight fade between
    float t = mod(iTime, 4.0);
    float tOff = mod(iTime + 2.0, 4.0);
    vec2 newUV = map.rg * bound(2.0, 3.0, 0.20, tOff)
               + map.bg * bound(1.0, 2.0, 0.20, t)
               + map.br * bound(2.0, 3.0, 0.20, t)
               + map.gr * bound(1.0, 2.0, 0.20, tOff);

    newUV = mix(uv, newUV, iIntensity * map.a);
    fragColor = texture(iInput, newUV);
}
