#property description Turns the image into tiles and flips each one along a diagonal

fn main(uv: vec2<f32>) -> vec4<f32> {
    let timeOffset = vec2<f32>(iTime * iFrequency * (1. / 64.)) * sign(uv.x + uv.y - 1.);
    let timeOffset = (timeOffset + 1.) % 2. - 1.;
    let newPt = (uv - 0.5 - timeOffset) * aspectCorrection;

    let bins = mix(50., 3., iIntensity);

    let newPtInt = floor(newPt * bins);
    let newPtFrac = fract(newPt * bins);

    let newPtFrac = 1. - newPtFrac.yx;

    let newPt = (newPtInt + newPtFrac) / bins;

    let fragColor = textureSample(iInputsTex[0], iSampler,  newPt / aspectCorrection + 0.5 + timeOffset);
    return mix(textureSample(iInputsTex[0], iSampler,  uv), fragColor, smoothstep(0., 0.1, iIntensity));
}
