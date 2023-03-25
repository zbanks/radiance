#property description Apply gaussian spatial blur

fn gaussian(x: f32) -> f32 {
    return exp(-0.5*x*x);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let sigma = iIntensity / 32. * pow(defaultPulse, 2.);
    var acc = vec4<f32>(0.);
    var norm = 0.;
    let stp = aspectCorrection.y * sigma;
    for (var i: f32 = -2.; i <= 2.; i+=(1./16.)) {
        let off = i * stp;
        let k = gaussian(i);
        norm += k;
        let pt = clamp(vec2<f32>(uv.x,uv.y + off), vec2<f32>(0.), vec2<f32>(1.));
        acc += k * textureSample(iChannelsTex[1], iSampler, pt);
    }
    return acc / norm;
}

#buffershader

fn gaussian(x: f32) -> f32 {
    return exp(-0.5*x*x);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let sigma = iIntensity / 32. * pow(defaultPulse, 2.);
    var acc = vec4<f32>(0.);
    var norm = 0.;
    let stp = aspectCorrection.x * sigma;
    for (var i: f32 = -2.; i <= 2.; i+=(1./16.)) {
        let off = i * stp;
        let pt = clamp(vec2(uv.x + off,uv.y), vec2<f32>(0.), vec2<f32>(1.));
        let k = gaussian(i);
        norm += k;
        acc += k * textureSample(iInputsTex[0], iSampler, pt);
    }
    return acc / norm;
}
