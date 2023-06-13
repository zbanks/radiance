#property description Saturate colors in YUV space by making things more UV

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = demultiply(fragColor);
    let yuv = rgb2yuv(c.rgb);

    let u_v = yuv.gb;
    let d = u_v - vec2<f32>(0.5, 0.5);
    let u_v = u_v + d * iIntensity * 3.0;
    let u_v = clamp(u_v, vec2<f32>(0.), vec2<f32>(1.));

    let rgb = yuv2rgb(vec3<f32>(yuv.x, u_v));
    let c = vec4<f32>(rgb, 1.) * c.a;
    return mix(fragColor, c, pow(defaultPulse, 2.));
}
