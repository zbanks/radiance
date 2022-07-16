//#property description Organic purple waves
//#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection + 0.5;

    let parameter = iTime * iFrequency;

    let y = pow(sin(cos(parameter / 4.) * normCoord.y * 8. + normCoord.x), 2.);
    let x = (sin(normCoord.x * 4.) + cos(normCoord.y * normCoord.x * 5.) * (y * 0.2 + 0.8) + 3.0) % 1.0;

    var c = vec4<f32>();
    c.r = mix(x, y, 0.3);
    c.b = pow(mix(x, y, 0.7), 0.6);
    c.g = 0.;
    c.a = 1.;
    c *= iIntensity;

    c = composite(textureSample(iInputsTex[0], iSampler, uv), c);
    return c;
}
