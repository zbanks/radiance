#property description YUV color channel separation
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let spin = iTime * 0.2;
    let separate = iIntensity * 0.1 * cos(iTime * iFrequency * pi * 0.25);
    let normCoord = (uv - 0.5) * aspectCorrection;

    let yOffset = normCoord - separate * vec2<f32>(cos(spin), sin(spin));
    let uOffset = normCoord - separate * vec2<f32>(cos(2. + spin), sin(2. + spin));
    let vOffset = normCoord - separate * vec2<f32>(cos(4. + spin), sin(4. + spin));

    let yImage = textureSample(iInputsTex[0], iSampler, yOffset / aspectCorrection + 0.5);
    let uImage = textureSample(iInputsTex[0], iSampler, uOffset / aspectCorrection + 0.5);
    let vImage = textureSample(iInputsTex[0], iSampler, vOffset / aspectCorrection + 0.5);

    let yuv = vec3<f32>(0.);
    let y = rgb2yuv(demultiply(yImage).rgb).r;
    let u = rgb2yuv(demultiply(uImage).rgb).g;
    let v = rgb2yuv(demultiply(vImage).rgb).b;

    // hmmm alpha is hard #TODO
    //vec3<f32> rgb = vec3<f32>(yImage.r, uImage.g, vImage.b);
    //float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    
    let rgb = yuv2rgb(vec3<f32>(y, u, v));
    //float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    let a_out = yImage.a;

    return premultiply(vec4<f32>(rgb, a_out));
}
