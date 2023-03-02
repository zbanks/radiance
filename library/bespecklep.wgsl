#property description Brownian-ish speckle effect with perlin noise

fn main(uv: vec2<f32>) -> vec4<f32> {
    let old_c = textureSample(iChannelsTex[0], iSampler, uv);
    let new_c = textureSample(iInputsTex[0], iSampler, uv);

    let r = noise3(vec3<f32>(uv * 16., iTime * iFrequency));
    let k = pow(mix(1.0, r, iIntensity), 2.0);
    let c = mix(old_c, new_c, k);

    // I don't think this is required, but just be safe
    return clamp(c, vec4<f32>(0.0), vec4<f32>(c.a));
}

