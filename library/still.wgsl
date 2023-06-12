// I don't know what this is supposed to do

#property description Stick chunks to the screen
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let hold = textureSample(iChannelsTex[1], iSampler,  uv);
    let inp = textureSample(iInputsTex[0], iSampler,  uv);
    let hold = vec4<f32>(hold.rgb, 1.) * inp.a;
    return mix(inp, hold, iIntensity);
}

#buffershader

fn main(uv: vec2<f32>) -> vec4<f32> {
    let inp = textureSample(iInputsTex[0], iSampler,  uv);
    let hold = textureSample(iChannelsTex[1], iSampler,  uv);

    let k = hold.a;
    let d = distance(inp.rgb, hold.rgb) / sqrt(3.);

    let k = k + pow(d, 0.5) * 0.3 - 0.03;
    let k = k * pow(iIntensity, 0.3);
    let k = k + pow(defaultPulse, 0.5); // I don't really get whats going on here
    let k = clamp(k, 0., 1.);

    let rgb = mix(inp.rgb, hold.rgb, k);
    let a = k;
    return vec4<f32>(rgb, a);
}
