#property description Brightly highlight pixels that are NAN or INF

fn is_p_inf(x: f32) -> bool {
    return x > 0. && x * 2. == x;
}

fn is_n_inf(x: f32) -> bool {
    return x < 0. && x * 2. == x;
}

fn is_nan(x: f32) -> bool {
    return !(x >= 0. || x <= 0.);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);

    // NAN / INF check:
    //let isnan = (
    //    c.r * 0. != 0.
    //    || c.g * 0. != 0.
    //    || c.b * 0. != 0.
    //    || c.a * 0. != 0.
    //);

    let any_is_p_inf = is_p_inf(c.a) || is_p_inf(c.r) || is_p_inf(c.g) || is_p_inf(c.b);
    let any_is_n_inf = is_n_inf(c.a) || is_n_inf(c.r) || is_n_inf(c.g) || is_n_inf(c.b);
    let any_is_nan = is_nan(c.a) || is_nan(c.r) || is_nan(c.g) || is_nan(c.b);

    if (iIntensity > 0.1) {
        if any_is_n_inf {
            return vec4<f32>(1., 1., 0., 1.); // Yellow = negative INF
        } else if any_is_p_inf {
            return vec4<f32>(0., 1., 1., 1.); // Cyan = positive INF
        } else if any_is_nan {
            return vec4<f32>(1., 0., 1., 1.); // Magenta = NAN
        } else {
            return vec4<f32>(0., 0., 0., 1.);
        }
    } else {
        return c;
    }
}
