#property description Scramble up input blocks

fn main(uv: vec2<f32>) -> vec4<f32> {
    let secondary = textureSample(iChannelsTex[1], iSampler,  uv);
    let uvNew = mix(uv, secondary.xy, smoothstep(0.0, 0.2, iIntensity));
    return textureSample(iInputsTex[0], iSampler,  uvNew);
}

#buffershader

fn pixelate(xy: vec2<f32>, n_buckets: f32) -> vec2<f32> {
    let xy_buckets = n_buckets * aspectCorrection;
    let xy = xy - 0.5;
    let xy = round(xy * xy_buckets) / xy_buckets;
    let xy = xy + 0.5;
    return xy;
}

fn in_bucket(uv: vec2<f32>, xy: vec2<f32>, n_buckets: f32) -> bool{
    let d = abs(uv - xy) * n_buckets;
    return max(d.x, d.y) <= 0.5;
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let n_buckets = 10.;

    let left = vec2<f32>(rand2(vec2<f32>(iTime, 0.)), rand2(vec2<f32>(iTime, 1.)));
    let left = pixelate(left, n_buckets);
    let right = vec2<f32>(rand2(vec2<f32>(iTime, 2.)), rand2(vec2<f32>(iTime, 3.)));
    let right = pixelate(right, n_buckets);

    var newColor = textureSample(iChannelsTex[1], iSampler,  uv);
    if (in_bucket(uv, left, n_buckets)) {
        let newCoord = uv - left + right;
        let oldDist = distance(newColor.xy, uv);
        let newDist = distance(newCoord, uv);
        let improvement = oldDist - newDist; // positive means reverting to normal
        if (improvement > -1.8 * iIntensity + 0.5) {
            // Scramble more
            newColor = vec4<f32>(newCoord, 1., 1.);
        }
        if (iIntensity < 0.3) {
            // Revert: Scramble less
            newColor = vec4<f32>(uv.xy, 1., 1.);
        }
    }

    let erase = pow(1. - sawtooth(iTime * iFrequency * 0.25, 0.1), 10.);

    return mix(
        vec4<f32>(uv.xy, 1., 1.),
        newColor,
        step(0.05, iIntensity) * (1. - erase)
    );
}
