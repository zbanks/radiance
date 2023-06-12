#property description Apply gaussian resampling ( at pixel aligned points )

fn squared(x: f32) -> f32 { return x * x; }

fn gaussian(x: f32, sigma: f32) -> f32 {
    return select(
        exp(-0.5*squared(x/sigma)),
        select(0., 1., x==0.),
        sigma == 0.
    );
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let sigma = iIntensity * 16. * defaultPulse;
    var acc = vec4<f32>(0.);
    var norm = 0.;
    let stp = onePixel.y;
    for(var i = -16.; i <= 16.; i+=1.) {
        let off = i * stp;
        let pt = clamp(vec2<f32>(uv.x,uv.y + off),vec2<f32>(0.),vec2<f32>(1.));

        let k = gaussian(i,sigma);
        norm += k;
        acc += k * textureSample(iChannelsTex[1], iSampler, pt);
    }
    return acc / norm;
}
#buffershader
fn squared(x: f32) -> f32 { return x * x; }

fn gaussian(x: f32, sigma: f32) -> f32 {
    return select(
        exp(-0.5*squared(x/sigma)),
        select(0., 1., x==0.),
        sigma == 0.
    );
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let sigma = iIntensity * 16. * defaultPulse;
    var acc = vec4<f32>(0.);
    var norm = 0.;
    let stp = onePixel.x;
    for(var i = -16.; i <= 16.; i+=1.) {
        let off = i * stp;
        let pt = clamp(vec2(uv.x + off,uv.y),vec2<f32>(0.),vec2<f32>(1.));

        let k = gaussian(i,sigma);
        norm += k;
        acc += k * textureSample(iInputsTex[0], iSampler, pt);
    }
    return acc / norm;
}
