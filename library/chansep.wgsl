#property description Red / green / blue color channel separation
#property frequency 1
// AKA "Chromatic Aberration"

fn main(uv: vec2<f32>) -> vec4<f32> {
    let spin = iTime * 0.25;
    let separate = iIntensity * 0.1 * cos(iTime * pi * 0.25 * iFrequency);
    let normCoord = (uv - 0.5) * aspectCorrection;
    let redOffset = normCoord - separate * vec2<f32>(cos(spin), sin(spin));
    let greenOffset = normCoord - separate * vec2<f32>(cos(2. + spin), sin(2. + spin));
    let blueOffset = normCoord - separate * vec2<f32>(cos(4. + spin), sin(4. + spin));

    let redImage = textureSample(iInputsTex[0], iSampler, redOffset / aspectCorrection + 0.5);
    let greenImage = textureSample(iInputsTex[0], iSampler, greenOffset / aspectCorrection + 0.5);
    let blueImage = textureSample(iInputsTex[0], iSampler, blueOffset / aspectCorrection + 0.5);

    let rgb = vec3<f32>(redImage.r, greenImage.g, blueImage.b);
    let a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    return vec4<f32>(rgb, a_out);
}

