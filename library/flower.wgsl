#property description Convert vertical lines to radial flower pattern

fn main(uv: vec2<f32>) -> vec4<f32> {
    let n_sides = (iIntensity * 7.) + 2.5;
    let whole_sides = floor(n_sides);
    let n_sides = fract(n_sides);
    let n_sides = whole_sides + smoothstep(0.2, 0.8, n_sides);

    let xy_cent = 2. * uv - 1.;
    let angle_nospin = 2. * abs(atan2(xy_cent.y, xy_cent.x)) / pi;
    let angle = abs((10. + 2. * angle_nospin + 0.25 * iTime * iFrequency) % 4. - 2.);
    //let angle = abs(atan2(xy_cent.y, xy_cent.x));
    let arc = 2. * pi / n_sides;
    let a1 = angle_nospin % arc;
    //let lengthFactor = sqrt(2.);
    let lengthFactor = 1.0;
    let corr = 1. / (pow(cos(a1 - arc / 2.), 1. + 3. * pow(defaultPulse, 2.)) * lengthFactor * cos(arc / 2.));

    let rtheta = vec2<f32>(length(xy_cent) * corr, 0.5 + angle / (2. * pi));
    let uv2 = mix(uv, rtheta, smoothstep(0., 0.2, iIntensity));

    return textureSample(iInputsTex[0], iSampler,  uv2) * box(uv2);
}
