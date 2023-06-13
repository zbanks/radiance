#property description Apply `uvmap` using 1 input for both UV & RGB

fn main(uv: vec2<f32>) -> vec4<f32> {
    let map = textureSample(iInputsTex[0], iSampler,  uv);
    let newUV = mix(uv, map.rg, iIntensity * map.a * pow(defaultPulse, 2.));
    return textureSample(iInputsTex[0], iSampler,  newUV);
}
