#property description Derivative of https://www.shadertoy.com/view/XssGD7, with tighter edges.

fn get_texture(uv: vec2<f32>, off: vec2<f32>, cor: vec2<f32>) -> vec4<f32> {
    return textureSample(iInputsTex[0], iSampler, uv + off * cor);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Sobel operator
    let off = max(0.0001, 8. * (1. - iIntensity * defaultPulse) + 1.);
    let cor = 1. / iResolution.xy;
    let o = vec3<f32>(-off, 0.0, off);
    let gx = vec4<f32>(0.0);
    let gy = vec4<f32>(0.0);
    let gx = gx + get_texture(uv, o.xz, cor);
    let gy = gy + gx;
    let gx = gx + 2.0 * get_texture(uv, o.xy, cor);
    let t = get_texture(uv, o.xx, cor);
    let gx = gx + t;
    let gy = gy - t;
    let gy = gy + 2.0 * get_texture(uv, o.yz, cor);
    let gy = gy - 2.0 * get_texture(uv, o.yx, cor);
    let t = get_texture(uv, o.zz, cor);
    let gx = gx - t;
    let gy = gy + t;
    let gx = gx - 2.0 * get_texture(uv, o.zy,cor);
    let t = get_texture(uv, o.zx,cor);
    let gx = gx - t;
    let gy = gy - t;

    let grad = sqrt(gx * gx + gy * gy);
    let grad = vec4<f32>(grad.xyz / sqrt(off), grad.a);
    let grad = vec4<f32>(grad.xyz, max(max(grad.r, grad.g), max(grad.b, grad.a)));

    let original = textureSample(iInputsTex[0], iSampler, uv);

    return mix(original, grad, smoothstep(0., 0.3, iIntensity));
}

