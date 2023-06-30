#property description Replace -inf and NAN with 0, and +inf with 1

fn fix(x: f32) -> f32 {
    if !(x > 0.) {
        return 0.;
    } else if !(x < 1.) {
        return 1.;
    } else {
        return x;
    }
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSampleLevel(iInputsTex[0], iSampler, uv, 0.);

    let fixed_c = vec4<f32>(fix(c.r), fix(c.g), fix(c.b), fix(c.a));

    return select(c, fixed_c, iIntensity > 0.1);
}
