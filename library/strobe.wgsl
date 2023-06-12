#property description Strobe alpha to the beat
#property frequency 1

// TODO: This effect does nothing at frequency 0

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return fragColor * (pow(defaultPulse, iIntensity * 5.));
}
