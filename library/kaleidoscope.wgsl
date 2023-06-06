#property description Mirror and repeat the pattern in a circle
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5);
    let normCoord = normCoord * (aspectCorrection);
    let r = length(normCoord);
    let theta = atan2(normCoord.y, normCoord.x);

    let bins = iIntensity * 5. + 2.;
    let tStep = pi / bins;
    let theta = abs((theta + 10. * tStep) % (2. * tStep) - tStep);

    let theta = theta + (iTime * iFrequency * pi * 0.125) % pi;

    let newUV = r * vec2<f32>(cos(theta), sin(theta));
    let newUV = newUV * (0.707);
    let newUV = newUV / (aspectCorrection);
    let newUV = newUV * 0.5 + 0.5;

    return textureSample(iInputsTex[0], iSampler,  mix(uv, newUV, smoothstep(0., 0.2, iIntensity)));
}
