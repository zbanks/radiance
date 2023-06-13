#property description Identity input for yuvmap

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Y could be anything, use 0.5
    let scaledUV = (uv - 0.5) / 4.0 + 0.5;
    let yuv = vec3<f32>(0.5, scaledUV);

    let base = textureSample(iInputsTex[0], iSampler,  uv);
    let yuvColor = clamp(vec4<f32>(yuv2rgb(yuv), 1.0), vec4<f32>(0.0), vec4<f32>(1.0));
    return mix(base, yuvColor, iIntensity * pow(defaultPulse, 2.));
}

