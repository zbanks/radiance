#property description Switch between the two inputs on the beat
#property inputCount 2
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let l = textureSample(iInputsTex[0], iSampler, uv);
    let r = textureSample(iInputsTex[1], iSampler, uv);

    let amt = mix(1., 8., iIntensity);
    let switcher = 0.5 * clamp(amt * sin(iTime * iFrequency * pi), -1., 1.) + 0.5;

    let which = mix(0., switcher, smoothstep(0., 0.1, iIntensity));
    let which = mix(which, 1., smoothstep(0.9, 1., iIntensity));

    return mix(l, r, which);
}
