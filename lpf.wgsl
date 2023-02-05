// TODO this is a pared-down version of LPF. The real version involves multiple buffer shaders.

fn main(uv: vec2<f32>) -> vec4<f32> {
    let a = 1. - iIntensity;
    let fragColor = mix(textureSample(iChannelsTex[0], iSampler, uv), textureSample(iInputsTex[0], iSampler, uv), a);
    return fragColor;
}
