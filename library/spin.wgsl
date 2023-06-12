#property description Spins the pattern round to the beat
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {

    let r = select(0., fract(iTime * iFrequency), iFrequency >  0.);

    let normCoord = (uv - 0.5) * aspectCorrection;

    let s = sin(r * pi);
    let c = cos(r * pi);
    let rot = mat2x2<f32>(c, -s, s, c);

    let newUV = normCoord * rot / aspectCorrection;
    let newUV = newUV * min(iResolution.x, iResolution.y) / max(iResolution.x, iResolution.y);
    let newUV = newUV + 0.5;

    let oc = textureSample(iInputsTex[0], iSampler,  uv);
    let nc = textureSample(iInputsTex[0], iSampler,  newUV);
    let nc = nc * box(newUV);

    return mix(oc, nc, smoothstep(0., 0.5, iIntensity));
}
