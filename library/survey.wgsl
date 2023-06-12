#property description Zoom in and pan across the surface
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let t = iTime * iFrequency *  pi / 32.;
    let sweep = vec2<f32>(cos(3. * t), sin(2. * t));

    let amount = iIntensity * 0.5;

    let factor = (1. - 2. * amount);

    let newUV = (uv - 0.5) * factor + sweep * amount + 0.5;

    return textureSample(iInputsTex[0], iSampler,  newUV) * box(newUV);
}
