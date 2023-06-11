#property description Pixelate/quantize the output

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    let bs = 256. * pow(2., -9. * iIntensity);
    let bs = bs * (0.7 + 0.3 * pow(defaultPulse, 2.));
    let bins = bs * aspectCorrection;
    let normCoord = round(normCoord * bins) / bins;

    let newUV = normCoord / aspectCorrection + 0.5;

    return textureSample(iInputsTex[0], iSampler,  newUV);
}
