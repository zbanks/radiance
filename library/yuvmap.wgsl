#property description Use UV (from YUV) as .uv

#property inputCount 2
fn main(uv: vec2<f32>) -> vec4<f32> {
    let map = textureSample(iInputsTex[1], iSampler,  uv);
    let yuv = rgb2yuv(demultiply(map).rgb);
    let scaledUV = (yuv.gb - 0.5) * 4.0 + 0.5;
    let newUV = mix(uv, scaledUV, iIntensity * map.a * pow(defaultPulse, 2.));
    return textureSample(iInputsTex[0], iSampler,  newUV);
}
