#property description Shift the color in YUV space by rotating on the UV plane

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let yuv = rgb2yuv(demultiply(fragColor).rgb);

    let t = select(iIntensity * 2. * pi, iTime * iFrequency * pi, iFrequency != 0.);
    let u_v = yuv.gb * 2.;
    let u_v = u_v - 1.;
    let u_v = vec2<f32>(u_v.x * cos(t) - yuv.b * sin(t),
                        u_v.y * sin(t) + yuv.b * cos(t));
    let u_v = u_v + 1.;
    let u_v = u_v / 2.;

    let yuv = vec3<f32>(yuv.r, u_v);

    return select(vec4<f32>(yuv2rgb(yuv), 1.) * fragColor.a, mix(fragColor, vec4<f32>(yuv2rgb(yuv), 1.) * fragColor.a, iIntensity), iFrequency != 0.);
}
