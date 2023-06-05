#property description From https://www.shadertoy.com/view/XssGD7

fn get_texture(uv: vec2<f32>) -> vec4<f32> {
    return textureSample(iInputsTex[0], iSampler,  uv);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
	// Sobel operator
	let offx = onePixel.x;
	let offy = onePixel.y;
	let gx = vec4<f32>(0.0);
	let gy = vec4<f32>(0.0);
	let gx = gx + get_texture(uv + vec2<f32>(-offx, offy));
	let gy = gy + gx;
	let gx = gx + 2.0*get_texture(uv + vec2<f32>(-offx, 0.));
	let t = get_texture(uv + vec2<f32>(-offx, -offy));
	let gx = gx + t;
	let gy = gy - t;
	let gy = gy + 2.0*get_texture(uv + vec2<f32>(0., offy));
	let gy = gy - 2.0*get_texture(uv + vec2<f32>(0., -offy));
	let t = get_texture(uv + vec2<f32>(offx, offy));
	let gx = gx - t;
	let gy = gy + t;
	let gx = gx - 2.0*get_texture(uv + vec2<f32>(offx, 0.));
	let t = get_texture(uv + vec2<f32>(-offx, offy));
	let gx = gx - t;
	let gy = gy - t;
	let grad = sqrt(gx * gx + gy * gy);
    let grad_a = max(max(grad.r, grad.g), max(grad.b, grad.a));
    let grad = vec4<f32>(grad.xyz, grad_a);

    let original = textureSample(iInputsTex[0], iSampler,  uv);
    let parameter = iIntensity * pow(defaultPulse, 2.);
    let grad = grad * smoothstep(0., 0.5, parameter);
    let original = original * (1. - smoothstep(0.5, 1., parameter));

    return composite(original, grad);
}
