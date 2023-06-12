#property description White slit for testing

fn main(uv: vec2<f32>) -> vec4<f32> {
    let xc = iIntensity;
    let color = vec4<f32>(1.0, 1.0, 1.0, 1.0);
    let color = color * (1. - step(0.5 * onePixel.x, abs(xc - uv.x)));
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return composite(fragColor, color);
}
