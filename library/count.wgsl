#property description Count out the beats
#property frequency 1
// This looks really dumb if BTrack doesn't do a good job :/

fn dist(pt: vec2<f32>, xbound: vec2<f32>, y: f32) -> f32 {
    // L1 distance function from a point to a X axis-aligned line segment
    // Line segment has endpoints `(xbound.x, y), (xbound.y, y)`

    // Distance from line
    let d = abs(pt.y - y);

    // L1 distance from endpoints (to give sharp, diamond corners)
    let d = d + step(pt.x, xbound.x) * (xbound.x - pt.x);
    let d = d + step(xbound.y, pt.x) * (pt.x - xbound.y);

    return d;
}

fn bound(low: f32, high: f32, eps: f32, t: f32) -> f32 {
    // Smooth 'box'-like function; returns 1.0 if t in [low, high] and 0.0 if outside
    // eps is a smoothing factor
    return smoothstep(low - eps, low + eps, t) - smoothstep(high - eps, high + eps, t);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    // 7-seg display; not all segments are actually used
    //
    //  AAA
    // F   B
    // F   B
    //  GGG
    // E   C
    // E   C
    //  DDD

    let a = dist(uv.xy, vec2(0.40, 0.60), 0.20);
    let b = dist(uv.yx, vec2(0.25, 0.45), 0.65);
    let c = dist(uv.yx, vec2(0.55, 0.75), 0.65);
    let d = dist(uv.xy, vec2(0.40, 0.60), 0.80);
    let e = dist(uv.yx, vec2(0.55, 0.75), 0.35);
    let f = dist(uv.yx, vec2(0.25, 0.45), 0.35);
    let g = dist(uv.xy, vec2(0.40, 0.60), 0.50);

    // Font for 4 digits
    let one = min(b, c);
    let two = min(min(a, b), min(d, min(e, g)));
    let three = min(min(a, b), min(c, min(d, g)));
    let four = min(min(b, c), min(f, g));

    // Cycle through 4 digits based on beat counter, slight fade between #s
    let t = (iTime * iFrequency) % 4.0;
    let tOff = (iTime * iFrequency + 2.0) % 4.0;
    let totalDist = one   * bound(2.0, 3.0, 0.10, tOff) 
                    + two   * bound(1.0, 2.0, 0.10, t) 
                    + three * bound(2.0, 3.0, 0.10, t)
                    + four  * bound(1.0, 2.0, 0.10, tOff);

    let alpha = 1.0 - smoothstep(0.02, 0.04, totalDist);
    let alpha = clamp(0., 1., alpha);

    let color = vec4(1.0, 0.2, 0.0, 1.0); // Red
    let color = color * alpha;
    let color = color * iIntensity;

    return composite(textureSample(iInputsTex[0], iSampler, uv), color);
}
