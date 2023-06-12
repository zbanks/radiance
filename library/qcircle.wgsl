#property description Big purple soft circle 
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let t = iTime * iFrequency * pi / 4.;
    let center = vec2<f32>(sin(t), cos(t));
    let center = center * (0.5);

    let a = clamp(length(center - normCoord), 0., 1.);
    let a = pow(a, 2.);
    let a = 1.0 - a;
    let a = a * (iIntensity);

    let c = vec4<f32>(0.2, 0.1, 0.5, 1.) * a;

    return composite(textureSample(iInputsTex[0], iSampler,  uv), c);
}
