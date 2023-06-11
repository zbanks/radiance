#property description Convert to grayscale/greyscale/monochrome/black & white

fn main(uv: vec2<f32>) -> vec4<f32> {
    let original = textureSample(iInputsTex[0], iSampler,  uv);
    let y = dot(vec3<f32>(0.2627, 0.6780, 0.0593), original.rgb);
    let bw = vec4<f32>(y, y, y, original.a);
    return mix(original, bw, iIntensity * pow(defaultPulse, 2.0));
}
