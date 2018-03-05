#property description Count out the beats
#property frequency 1
// This looks really dumb if BTrack doesn't do a good job :/

float dist(vec2 point, vec2 xbound, float y) {
    // L1 distance function from a point to a X axis-aligned line segment
    // Line segment has endpoints `(xbound.x, y), (xbound.y, y)`

    // Distance from line
    float d = abs(point.y - y);

    // L1 distance from endpoints (to give sharp, diamond corners)
    d += step(point.x, xbound.x) * (xbound.x - point.x);
    d += step(xbound.y, point.x) * (point.x - xbound.y);

    return d;
}

float bound(float low, float high, float eps, float t) {
    // Smooth 'box'-like function; returns 1.0 if t in [low, high] and 0.0 if outside
    // eps is a smoothing factor
    return smoothstep(low - eps, low + eps, t) - smoothstep(high - eps, high + eps, t);
}

void main(void) {
    // 7-seg display; not all segments are actually used
    //
    //  AAA
    // F   B
    // F   B
    //  GGG
    // E   C
    // E   C
    //  DDD

    float a = dist(uv.xy, vec2(0.40, 0.60), 0.80);
    float b = dist(uv.yx, vec2(0.55, 0.75), 0.65);
    float c = dist(uv.yx, vec2(0.25, 0.45), 0.65);
    float d = dist(uv.xy, vec2(0.40, 0.60), 0.20);
    float e = dist(uv.yx, vec2(0.25, 0.45), 0.35);
    float f = dist(uv.yx, vec2(0.55, 0.75), 0.35);
    float g = dist(uv.xy, vec2(0.40, 0.60), 0.50);

    // Font for 4 digits
    float one = min(b, c);
    float two = min(min(a, b), min(d, min(e, g)));
    float three = min(min(a, b), min(c, min(d, g)));
    float four = min(min(b, c), min(f, g));

    // Cycle through 4 digits based on beat counter, slight fade between #s
    float t = mod(iTime * iFrequency, 4.0);
    float tOff = mod(iTime * iFrequency + 2.0, 4.0);
    float totalDist = one   * bound(2.0, 3.0, 0.10, tOff) 
                    + two   * bound(1.0, 2.0, 0.10, t) 
                    + three * bound(2.0, 3.0, 0.10, t)
                    + four  * bound(1.0, 2.0, 0.10, tOff);

    float alpha = 1.0 - smoothstep(0.02, 0.04, totalDist);
    alpha = clamp(0., 1., alpha);

    vec4 color = vec4(1.0, 0.2, 0.0, 1.0); // Red
    color *= alpha;
    color *= iIntensity;

    fragColor = composite(texture(iInput, uv), color);
}
