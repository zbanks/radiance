#property description Recolor output with perlin noise rainbow
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let factor = pow(iIntensity, 0.6);
    let noise_input = vec3<f32>(uv, iTime * iFrequency / 8.);
    let n = noise3(noise_input) - 0.1;
    let n = n + (noise3(2. * noise_input) - 0.5) * 0.5;
    let n = n + (noise3(4. * noise_input) - 0.5) * 0.25;
    //let n = mod(n + 0.5, 1.0);
    //let n = mod(n + hsl.r, 1.0);

    let samp = textureSample(iInputsTex[0], iSampler,  uv);
    let hsl = rgb2hsv(samp.rgb);
    let s = 1.0 - (1.0 - hsl.g) * (1.0 - factor);
    //let hsl.r = mix(hsl.r, n, iIntensity);
    let h = fract(hsl.r + n * iIntensity);
    let l = hsl.b;
    return add_alpha(hsv2rgb(vec3<f32>(h, s, l)), samp.a);
}
