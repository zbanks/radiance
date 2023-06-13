#property description Cycle through different UV map coordinates
#property inputCount 2
#property frequency 1

fn bound(low: f32, high: f32, eps: f32, t: f32) -> f32 {
    // Smooth 'box'-like function; returns 1.0 if t in [low, high] and 0.0 if outside
    // eps is a smoothing factor
    return smoothstep(low - eps, low + eps, t) - smoothstep(high - eps, high + eps, t);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let map = textureSample(iInputsTex[1], iSampler,  uv);

    // Cycle through 4 states based on beat counter, slight fade between
    let t = (iTime * iFrequency) % 4.0;
    let tOff = (iTime * iFrequency + 2.0) % 4.0;
    let newUV = map.rg * bound(2.0, 3.0, 0.20, tOff)
               + map.bg * bound(1.0, 2.0, 0.20, t)
               + map.br * bound(2.0, 3.0, 0.20, t)
               + map.gr * bound(1.0, 2.0, 0.20, tOff);

    let newUV = mix(uv, newUV, iIntensity * map.a);
    return textureSample(iInputsTex[0], iSampler,  newUV);
}
