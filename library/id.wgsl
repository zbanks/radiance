#property description Pass the input through unaltered

fn main(uv: vec2<f32>) -> vec4<f32> {
    return textureSample(iInputsTex[0], iSampler,  uv);
}

