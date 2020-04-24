#property description CMYK halftoning, like a printer

vec4 rgb2cmyk(vec3 rgb) {
    vec4 cmyk;
    cmyk.w = 1. - max(max(rgb.r, rgb.g), rgb.b);
    cmyk.xyz = (1. - rgb - cmyk.w) / max(1. - cmyk.w, 0.001);
    return cmyk;
}

vec3 cmyk2rgb(vec4 cmyk) {
    return (1. - cmyk.xyz) * (1. - cmyk.w);
}

vec3 grid(mat2 basis, vec4 cmykMask, vec2 offset) {
    float points = 300. * pow(2., -9. * iIntensity) + 5.;
    float r = 0.5 / points;

    mat2 invBasis = inverse(basis);

    vec2 pt = (uv - 0.5) * aspectCorrection;

    vec2 newCoord = round(pt * points * invBasis - offset) + offset;
    vec2 colorCoord = newCoord / points * basis;
    vec3 c = texture(iInput, colorCoord / aspectCorrection + 0.5).rgb;
    vec4 cmyk = rgb2cmyk(c);
    cmyk *= cmykMask;
    float cmykValue = dot(cmyk, vec4(1.));
    r *= sqrt(cmykValue);
    cmyk /= max(cmykValue, 0.001);
    c = mix(vec3(1.), cmyk2rgb(cmyk), 1. - smoothstep(r * 0.8, r, length(pt - colorCoord)));
    return c;
}

mat2 basis(float t) {
    t = t * M_PI / 180.;
    return mat2(cos(t), sin(t),
                -sin(t), cos(t));
}

void main(void) {
    mat2 b1 = basis(15.);
    mat2 b2 = basis(75.);
    mat2 b3 = basis(0.);
    mat2 b4 = basis(45.);

    vec3 c1 = grid(b1, vec4(1., 0., 0., 0.), vec2(0.));
    vec3 m1 = grid(b2, vec4(0., 1., 0., 0.), vec2(0.));
    vec3 y1 = grid(b3, vec4(0., 0., 1., 0.), vec2(0.));
    vec3 k1 = grid(b4, vec4(0., 0., 0., 1.), vec2(0.));
    vec3 c2 = grid(b1, vec4(1., 0., 0., 0.), vec2(0.5));
    vec3 m2 = grid(b2, vec4(0., 1., 0., 0.), vec2(0.5));
    vec3 y2 = grid(b3, vec4(0., 0., 1., 0.), vec2(0.5));
    vec3 k2 = grid(b4, vec4(0., 0., 0., 1.), vec2(0.5));
    vec3 total = vec3(1.);
    total = min(total, c1);
    total = min(total, m1);
    total = min(total, y1);
    total = min(total, k1);
    total = min(total, c2);
    total = min(total, m2);
    total = min(total, y2);
    total = min(total, k2);
    vec4 final = vec4(total, 1.);

    fragColor = texture(iInput, uv);
    final.a = max(fragColor.a, max(total.r, max(total.g, total.b)));
    fragColor = mix(fragColor, final, smoothstep(0., 0.1, iIntensity));
}
