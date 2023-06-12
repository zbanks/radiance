#property description Slide the screen left-to-right
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let deviation = iTime * iFrequency * 0.5 - 0.5;
    let uv2 = (uv - 0.5) * aspectCorrection;
    let x = abs((uv2.x + deviation + 1.5) % 2. - 1.) - 0.5;
    let uv2 = vec2<f32>(x, uv2.y);
    let uv2 = uv2 / aspectCorrection + 0.5;

    let oc = textureSample(iInputsTex[0], iSampler,  uv);
    let c = textureSample(iInputsTex[0], iSampler,  uv2);

    let oc = oc * (1. - smoothstep(0.1, 0.2, iIntensity));
    let c = c * smoothstep(0., 0.1, iIntensity);

    return composite(oc, c);
}
