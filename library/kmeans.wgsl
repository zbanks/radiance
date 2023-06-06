#property description k-means clustering? (idk)

fn main(uv: vec2<f32>) -> vec4<f32> {
    let uvCluster = textureSample(iChannelsTex[1], iSampler,  uv).xy;
    let uvNew = mix(uv, uvCluster, clamp(iIntensity * 2., 0., 1.0));
    return textureSample(iInputsTex[0], iSampler,  uvNew);
    //return textureSample(iChannelsTex[1], iSampler,  uv);
}

#buffershader

fn distFn(x: vec4<f32>, y: vec4<f32>) -> f32 {
    let v = demultiply(x).xyz - demultiply(y).xyz;
    let v = abs(v);
    //float d = v.x + v.y + v.z;
    let d = length(v);
    return d;
    //return 1. / (0.0001 + d);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    //float q = pow(2.0, floor(rand(vec3<f32>(uv, iTime)) * 4));
    let q = 1.;
    let a = uv;
    let b = textureSample(iChannelsTex[1], iSampler,  uv).xy;
    //vec2<f32> a = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>( 5,  5)).xy;
    //vec2<f32> b = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>(-5, -5)).xy;
    let c = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>( q,  q)).xy;
    let d = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>( q, -q)).xy;
    let e = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>(-q,  q)).xy;
    let f = textureSample(iChannelsTex[1], iSampler,  uv + onePixel * vec2<f32>(-q, -q)).xy;

    let av = textureSample(iInputsTex[0], iSampler,  a);
    let bv = textureSample(iInputsTex[0], iSampler,  b);
    let cv = textureSample(iInputsTex[0], iSampler,  c);
    let dv = textureSample(iInputsTex[0], iSampler,  d);
    let ev = textureSample(iInputsTex[0], iSampler,  e);
    let fv = textureSample(iInputsTex[0], iSampler,  f);

    let mean = (av + bv + cv + dv + ev + fv) / 6.0;

    let ad = distFn(av, mean);
    let bd = distFn(bv, mean);
    let cd = distFn(cv, mean);
    let dd = distFn(dv, mean);
    let ed = distFn(ev, mean);
    let fd = distFn(fv, mean);

    let minDist = min(min(min(ad, bd), min(cd, dd)), min(ed, fd));

    let af = select(0., 1., ad == minDist);
    let bf = select(0., 1., bd == minDist);
    let cf = select(0., 1., cd == minDist);
    let df = select(0., 1., dd == minDist);
    let ef = select(0., 1., ed == minDist);
    let ff = select(0., 1., fd == minDist);
    let fsum = af + bf + cf + df + ef + ff;
    let uvNew = (a * af + b * bf + c * cf + d * df + e * ef + f * ff) / (af + bf + cf + df + ef + ff);

    return vec4<f32>(uvNew, min(1.0, fsum * 0.5), 1.0);
    //return mean;
}
