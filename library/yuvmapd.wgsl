#property description Use UV (from YUV) as delta-.uv

#property inputCount 2
fn main(uv: vec2<f32>) -> vec4<f32> {
    let map = textureSample(iInputsTex[1], iSampler,  uv);
    let yuv = rgb2yuv(demultiply(map).rgb);
    let scaledUV = (yuv.gb - 0.5) * 4.0;
    let newUV = uv + scaledUV * iIntensity * map.a * pow(defaultPulse, 2.);
    let newUV = clamp(newUV, vec2<f32>(0.), vec2<f32>(1.));
    return textureSample(iInputsTex[0], iSampler,  newUV);
}
