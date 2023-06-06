#property description Pixelate, but circles in a hexagonal grid

fn triGrid(basis: mat2x2<f32>, uv: vec2<f32>) -> vec4<f32> {
    let points = min(3. / iIntensity, 10000.);
    let points = points / (0.3 + 0.7 * pow(defaultPulse, 2.));
    let r = 0.5 / points;

    let invBasis = inverse2(basis);

    let pt = (uv - 0.5) * aspectCorrection;

    let newCoord = round(pt * points * invBasis);
    let colorCoord = newCoord / points * basis;
    let c = textureSample(iInputsTex[0], iSampler,  colorCoord / aspectCorrection + 0.5);
    let c = c * (1. - step(r, length(pt - colorCoord)));
    return c;
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let R3 = sqrt(3.) / 2.;
    let tri1 = mat2x2<f32>(1., 0.5,
                           0., R3);
    let tri2 = mat2x2<f32>(1., -0.5,
                           0., R3);
    let tri3 = mat2x2<f32>(-0.5, 0.5,
                           R3, R3);

    let c1 = triGrid(tri1, uv);
    let c2 = triGrid(tri2, uv);
    let c3 = triGrid(tri3, uv);
    let c = max(max(c1, c2), c3);

    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return mix(fragColor, c, smoothstep(0., 0.1, iIntensity));
}
