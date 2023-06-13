#property description Rectilinear distortion
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let shift = vec2<f32>(0.);
    let t = iTime * iFrequency + 1.;
    let shift = shift + cos(pi * normCoord) * sin(t * vec2<f32>(0.1, 0.13));
    let shift = shift + cos(pi * normCoord * 2.) * sin(t * vec2<f32>(0.33, -0.23)) / 2.;
    let shift = shift + cos(pi * normCoord * 3.) * sin(t * vec2<f32>(0.35, -0.53)) / 3.;
    let shift = shift + cos(pi * normCoord * 4.) * sin(t * vec2<f32>(-0.63, -0.20)) / 4.;
    let shift = shift + cos(pi * normCoord * 5.) * sin(t * vec2<f32>(-0.73, 0.44)) / 5.;
    let shift = shift + cos(pi * normCoord * 6.) * sin(t * vec2<f32>(-0.73, 0.74)) / 6.;
    let shift = shift + cos(pi * normCoord * 7.) * sin(t * vec2<f32>(-1.05, -0.52)) / 7.;
    let shift = shift + cos(pi * normCoord * 8.) * sin(t * vec2<f32>(1.45, -1.22)) / 8.;

    let shift = shift + sin(pi * normCoord * 5.) * sin(t * vec2<f32>(0.79, -0.47)) / 5.;
    let shift = shift + sin(pi * normCoord * 6.) * sin(t * vec2<f32>(0.33, 0.79)) / 6.;
    let shift = shift + sin(pi * normCoord * 7.) * sin(t * vec2<f32>(1.15, -0.53)) / 7.;
    let shift = shift + sin(pi * normCoord * 8.) * sin(t * vec2<f32>(-1.36, -1.12)) / 8.;

    let amount = 0.1 * iIntensity;

    return textureSample(iInputsTex[0], iSampler,  (normCoord + shift * amount) / aspectCorrection + 0.5);
}
