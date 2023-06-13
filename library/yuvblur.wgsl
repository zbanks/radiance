#property description Horizontal blur of UV channels to give an old-timey effect
// This effect isn't actually that good...

let MAX_DELTA = 0.04;

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let yuv = rgb2yuv(demultiply(fragColor).rgb);

    let delta = MAX_DELTA * iIntensity * pow(defaultPulse, 2.);
    let left = rgb2yuv(demultiply(textureSample(iInputsTex[0], iSampler,  uv + vec2<f32>(-delta, 0.))).rgb);
    let right = rgb2yuv(demultiply(textureSample(iInputsTex[0], iSampler,  uv + vec2<f32>(delta, 0.))).rgb);
    let u_v = mix(yuv.gb, (left.gb + right.gb) / 2.0, 0.7 * iIntensity);
    let u_v = clamp(u_v, vec2<f32>(0.), vec2<f32>(1.));

    return vec4<f32>(yuv2rgb(vec3<f32>(yuv.x, u_v)), 1.) * fragColor.a;
}
