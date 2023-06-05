#property description Excessive rainbow
#property frequency 1

// A radiance classic

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = vec4<f32>(hsv2rgb(vec3<f32>((uv.x + iTime * iFrequency * 0.5) % 1., 1., 1.)), 1.);
    return mix(fragColor, c, iIntensity);
}
