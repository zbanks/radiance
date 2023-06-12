#property description RGB noise

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let v = rand3(vec3<f32>(uv, iTime));
    //let v = textureSample(iNoiseTex, iSampler, uv).r;
    let c = vec4<f32>(vec3<f32>(v), 1.0);
    return mix(fragColor, c, iIntensity * pow(defaultPulse, 0.5));
}
