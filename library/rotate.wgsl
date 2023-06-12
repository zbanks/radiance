#property description Rotate the screen

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    let r = iIntensity;
    let r = r * ((sawtooth(iTime * iFrequency / 2.0, 0.5) - 0.5) * 2.);
    let s = sin(r * pi);
    let c = cos(r * pi);
    let rot = mat2x2<f32>(c, -s, s, c);

    let newUV = normCoord * rot / aspectCorrection + 0.5;

    let fragColor = textureSample(iInputsTex[0], iSampler,  newUV);
    return fragColor * (box(newUV));
}
