#property description Desaturate in YUV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let factor = pow(defaultPulse, 2.) * iIntensity;

    let samp = demultiply(textureSample(iInputsTex[0], iSampler,  uv));

    let yuv = rgb2yuv(samp.rgb);
    let color_y = yuv.r;
    let color_uv = yuv.gb - (0.5);
    let color_uv = color_uv * (1.0 - factor);
    let color_uv = color_uv + (0.5);
    let rgb = yuv2rgb(vec3<f32>(color_y, color_uv));

    let fragColor = premultiply(vec4<f32>(rgb, samp.a));
    return fragColor;
}
