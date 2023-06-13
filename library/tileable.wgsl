#property description Makes tileable

fn main(uv: vec2<f32>) -> vec4<f32> {
    let o1 = vec2<f32>(0.5, 0.5);
    let o2 = vec2<f32>(0.5, -0.5);
    let o3 = vec2<f32>(-0.5, -0.5);
    let o4 = vec2<f32>(-0.5, 0.5);

    let i0 = box(uv) * textureSample(iInputsTex[0], iSampler,  uv);
    let i1 = box(uv + o1) * textureSample(iInputsTex[0], iSampler,  uv + o1);
    let i2 = box(uv + o2) * textureSample(iInputsTex[0], iSampler,  uv + o2);
    let i3 = box(uv + o3) * textureSample(iInputsTex[0], iSampler,  uv + o3);
    let i4 = box(uv + o4) * textureSample(iInputsTex[0], iSampler,  uv + o4);

    return max(i0, iIntensity * max(i1, max(i2, max(i3, i4))));
}
