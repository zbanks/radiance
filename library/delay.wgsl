#property description Introduce a delay (and reduce resolution)

let SZ = 6.;    // Delay of SZ*SZ frames (36)
fn main(uv: vec2<f32>) -> vec4<f32> {
    let original = textureSample(iInputsTex[0], iSampler, uv);
    let shift = floor(iResolution / SZ) / iResolution;
    let uvNew = uv * shift;
    let uvNew = mix(uvNew, uv, smoothstep(0.8, 0.9, iIntensity));
    let delayed = textureSample(iChannelsTex[1], iSampler, uvNew);
    return mix(original, delayed, smoothstep(0., 0.2, iIntensity));
}

#buffershader

let SZ = 6.;    // Delay of SZ*SZ frames (36)
fn main(uv: vec2<f32>) -> vec4<f32> {
    let shift = floor(iResolution / SZ) / iResolution;
    let maxShift = shift * SZ;
    var uvNext = uv + vec2<f32>(shift.x, 0.);
    if (uvNext.x > maxShift.x) {
        uvNext.x -= maxShift.x;
        uvNext.y += shift.y;
    }
    var c = vec4<f32>(0.);
    if (uvNext.y > maxShift.y) {
        uvNext.y -= maxShift.y;
        uvNext /= shift;
        c = textureSampleLevel(iInputsTex[0], iSampler, uvNext, 0.);
    } else {
        c = textureSampleLevel(iChannelsTex[1], iSampler, uvNext, 0.);
    }
    return c;
}
