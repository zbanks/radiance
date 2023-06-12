#property description Vertical stripes with a twinkle effect
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let xv = round(uv.x * 20. * aspectCorrection.x); 
    let fragColor = textureSample(iChannelsTex[0], iSampler,  uv);
    let fragColor = fragColor * (exp(-iIntensity * iFrequency / 20.));

    let fragColor = select(
        fragColor,
        textureSample(iInputsTex[0], iSampler,  uv),
        rand2(vec2<f32>(xv, iTime)) < exp(-iIntensity * 4.)
    );

    return fragColor;
}
