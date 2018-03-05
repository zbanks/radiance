#property description Pixelate, but circles in a hexagonal grid

vec4 triGrid(mat2 basis) {
    float points = min(3. / iIntensity, 10000.);
    points /= 0.3 + 0.7 * pow(defaultPulse, 2.);
    float r = 0.5 / points;

    mat2 invBasis = inverse(basis);

    vec2 pt = (uv - 0.5) * aspectCorrection;

    vec2 newCoord = round(pt * points * invBasis);
    vec2 colorCoord = newCoord / points * basis;
    vec4 c = texture(iInput, colorCoord / aspectCorrection + 0.5);
    c *= 1. - step(r, length(pt - colorCoord));
    return c;
}

void main(void) {
    const float R3 = sqrt(3.) / 2.;
    mat2 tri1 = mat2(1., 0.5,
                     0., R3);
    mat2 tri2 = mat2(1., -0.5,
                     0., R3);
    mat2 tri3 = mat2(-0.5, 0.5,
                     R3, R3);

    vec4 c1 = triGrid(tri1);
    vec4 c2 = triGrid(tri2);
    vec4 c3 = triGrid(tri3);
    vec4 c = max(max(c1, c2), c3);

    fragColor = texture(iInput, uv);
    fragColor = mix(fragColor, c, smoothstep(0., 0.1, iIntensity));
}
