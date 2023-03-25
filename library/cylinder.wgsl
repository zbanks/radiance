#property description Wrap the parent texture on a spinning cylinder
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let angle = (iTime * iFrequency - 0.5) * pi;
    let angle = angle + 2. * asin(2. * (uv.x - 0.5));

    let x = modulo(angle / pi, 2.0);
    let x = x - 1.0;
    let x = abs(x);

    let new_uv = vec2(x, uv.y);
    let new_uv = mix(uv, new_uv, iIntensity);
    return textureSample(iInputsTex[0], iSampler, new_uv);
}
