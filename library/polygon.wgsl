#property description Convert vertical lines to polygon rings
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let n_sides = clamp(2.4 / (1. - iIntensity), 3., 10000.);
    let whole_sides = floor(n_sides);
    let fract_sides = fract(n_sides);
    let n_sides = whole_sides + smoothstep(0.2, 0.8, fract_sides);

    let normCoord = 2. * (uv - 0.5) * aspectCorrection;
    let angle = atan2(normCoord.y, normCoord.x);
    let arc = 2. * pi / n_sides;
    let a1 = (abs(angle) + iFrequency * iTime * 0.25 * arc) % arc;
    //let lengthFactor = sqrt(2.);
    let lengthFactor = 1.0;
    let corr = cos(a1 - arc / 2.) / (lengthFactor * cos(arc / 2.));

    let rtheta = vec2<f32>(length(normCoord) * corr, 0.5 + angle / (2. * pi));
    let uv2 = mix(uv, rtheta, smoothstep(0., 0.2, iIntensity));

    return textureSample(iInputsTex[0], iSampler,  uv2) * box(uv2);
}
