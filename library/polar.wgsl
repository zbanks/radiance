#property description Convert vertical lines to rings
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5) * aspectCorrection;

    let lengthFactor = sqrt(2.);
    //let lengthFactor = 1.0;

    let newUV = vec2<f32>(length(normCoord) / lengthFactor, abs(((atan2(normCoord.x, -normCoord.y) / pi + 1. + 0.25 * iTime * iFrequency) % 2.) - 1.)) - 0.5;
    let newUV = newUV / aspectCorrection + 0.5;

    return textureSample(iInputsTex[0], iSampler,  mix(uv, newUV, iIntensity));
}
