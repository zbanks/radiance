#property description A broken shader
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    return extureSample(iInputsTex[0], iSampler, uv);
}
