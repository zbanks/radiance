#property description Repeating tiles, flipped so that the pattern tesselates

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = uv - 0.5;
    let bins = pow(2., 4. * iIntensity);
    let bins = bins / (mix(1., (0.7 + 0.3 * pow(defaultPulse, 2.)), smoothstep(0., 0.2, iIntensity)));
    let newUV = normCoord * bins;
    let newUV = abs((newUV + 1.5 + 10.) % 2. - 1.) - 0.5;
    return textureSample(iInputsTex[0], iSampler,  newUV + 0.5);
}
