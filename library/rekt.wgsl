#property description Get (covered in random) rekt(angles)
#property frequency 2

fn main(uv: vec2<f32>) -> vec4<f32> {
    let under = textureSample(iInputsTex[0], iSampler,  uv);

    let over = textureSample(iChannelsTex[1], iSampler,  uv);
    let a = step(1. - iIntensity + 0.005, over.a);
    let over = over * a;

    return composite(under, over);
}

#buffershader

fn main(uv: vec2<f32>) -> vec4<f32> {
    let before = textureSample(iChannelsTex[1], iSampler,  uv);
    let a = max(before.a - 0.005, 0.);
    let before = before * a;

    let transVec = textureSample(iChannelsTex[2], iSampler,  vec2<f32>(0.));
    let trans = step(0.5, transVec.r - transVec.g) + step(0., -iFrequency);

    let xy = vec2<f32>(rand2(vec2<f32>(iTime, 0.)), rand2(vec2<f32>(iTime, 1.)));
    let wh = vec2<f32>(rand2(vec2<f32>(iTime, 2.)), rand2(vec2<f32>(iTime, 3.))) * 0.3 + 0.1;

    //vec4<f32> color = vec4<f32>(rand2(vec2<f32>(iTime, 4.)), rand2(vec2<f32>(iTime, 5.)), rand2(vec2<f32>(iTime, 6.)), 1.0);

    //vec3<f32> yuv = vec3<f32>(0.5, rand(vec2<f32>(iTime, 4.)), rand(vec2<f32>(iTime, 5.)));
    //vec4<f32> color = clamp(vec4<f32>(yuv2rgb(yuv), 1.0), 0.0, 1.0);

    let hsv = vec3<f32>(rand2(vec2<f32>(iTime, 4.)), 1.0, 1.0);
    let color = clamp(vec4<f32>(hsv2rgb(hsv), 1.0), vec4<f32>(0.0), vec4<f32>(1.0));

    let inside = box((uv - xy) / wh + 0.5) * trans;
    return mix(before, color, inside);
}

#buffershader
// This shader stores the last value of defaultPulse in the red channel
// and the current value in the green channel.

fn main(uv: vec2<f32>) -> vec4<f32> {
    let last = textureSample(iChannelsTex[2], iSampler,  vec2<f32>(0.)).g;
    return vec4<f32>(last, fract(iFrequency * iTime), 0., 1.);
}
