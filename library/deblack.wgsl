#property description Increase alpha ( undo "black" )

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);
    let a = (1. - iIntensity * pow(defaultPulse, 2.));
    let a = max(0.0001, max(a, c.a));
    let c = c / a;
    return c;
}
