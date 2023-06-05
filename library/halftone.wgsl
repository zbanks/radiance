#property description CMYK halftoning, like a printer

fn rgb2cmyk(rgb: vec3<f32>) -> vec4<f32> {
    let k = 1. - max(max(rgb.r, rgb.g), rgb.b);
    let cmy = (1. - rgb - k) / max(1. - k, 0.001);
    return vec4<f32>(cmy, k);
}

fn cmyk2rgb(cmyk: vec4<f32>) -> vec3<f32> {
    return (1. - cmyk.xyz) * (1. - cmyk.w);
}

fn grid(basis: mat2x2<f32>, cmykMask: vec4<f32>, uv: vec2<f32>, offset: vec2<f32>) -> vec3<f32> {
    let points = 300. * pow(2., -9. * iIntensity) + 5.;
    let points = points / (0.7 + 0.3 * pow(defaultPulse, 2.));
    let r = 0.5 / points;

    let invBasis = inverse2(basis);

    let pt = (uv - 0.5) * aspectCorrection;

    let newCoord = round(pt * points * invBasis - offset) + offset;
    let colorCoord = newCoord / points * basis;
    let c = textureSample(iInputsTex[0], iSampler,  colorCoord / aspectCorrection + 0.5).rgb;
    let cmyk = rgb2cmyk(c);
    let cmyk = cmyk * (cmykMask);
    let cmykValue = dot(cmyk, vec4<f32>(1.));
    let r = r * (sqrt(cmykValue));
    let cmyk = cmyk / (max(cmykValue, 0.001));
    let c = mix(vec3<f32>(1.), cmyk2rgb(cmyk), 1. - smoothstep(r * 0.8, r, length(pt - colorCoord)));
    return c;
}

fn basis(t: f32) -> mat2x2<f32> {
    let t = t * pi / 180.;
    return mat2x2(cos(t), sin(t),
                 -sin(t), cos(t));
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let b1 = basis(15.);
    let b2 = basis(75.);
    let b3 = basis(0.);
    let b4 = basis(45.);

    let c1 = grid(b1, vec4<f32>(1., 0., 0., 0.), uv, vec2<f32>(0.));
    let m1 = grid(b2, vec4<f32>(0., 1., 0., 0.), uv, vec2<f32>(0.));
    let y1 = grid(b3, vec4<f32>(0., 0., 1., 0.), uv, vec2<f32>(0.));
    let k1 = grid(b4, vec4<f32>(0., 0., 0., 1.), uv, vec2<f32>(0.));
    let c2 = grid(b1, vec4<f32>(1., 0., 0., 0.), uv, vec2<f32>(0.5));
    let m2 = grid(b2, vec4<f32>(0., 1., 0., 0.), uv, vec2<f32>(0.5));
    let y2 = grid(b3, vec4<f32>(0., 0., 1., 0.), uv, vec2<f32>(0.5));
    let k2 = grid(b4, vec4<f32>(0., 0., 0., 1.), uv, vec2<f32>(0.5));
    let total = vec3<f32>(1.);
    let total = min(total, c1);
    let total = min(total, m1);
    let total = min(total, y1);
    let total = min(total, k1);
    let total = min(total, c2);
    let total = min(total, m2);
    let total = min(total, y2);
    let total = min(total, k2);
    let total = vec4<f32>(total, 1.);

    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let a = max(fragColor.a, max(total.r, max(total.g, total.b)));
    let total = vec4<f32>(total.rgb, a);
    return mix(fragColor, total, smoothstep(0., 0.1, iIntensity));
}
