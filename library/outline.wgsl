#property description Apply black outline around edges
//#property author https://www.shadertoy.com/view/XssGD7 + zbanks

fn main(uv: vec2<f32>) -> vec4<f32> {
	// Sobel operator
	let o = vec3<f32>(-1., 0., 1.);
	let gx = vec4<f32>(0.0);
	let gy = vec4<f32>(0.0);
	let gx = gx + textureSample(iInputsTex[0], iSampler,  uv + o.xz * onePixel);
	let gy = gy + gx;
	let gx = gx + 2.0*textureSample(iInputsTex[0], iSampler,  uv + o.xy * onePixel);
	let t = textureSample(iInputsTex[0], iSampler,  uv + o.xx * onePixel);
	let gx = gx + t;
	let gy = gy - t;
	let gy = gy + 2.0*textureSample(iInputsTex[0], iSampler,  uv + o.yz * onePixel);
	let gy = gy - 2.0*textureSample(iInputsTex[0], iSampler,  uv + o.yx * onePixel);
	let t = textureSample(iInputsTex[0], iSampler,  uv + o.zz * onePixel);
	let gx = gx - t;
	let gy = gy + t;
	let gx = gx - 2.0*textureSample(iInputsTex[0], iSampler,  uv + o.zy * onePixel);
	let t = textureSample(iInputsTex[0], iSampler,  uv + o.zx * onePixel);
	let gx = gx - t;
	let gy = gy - t;
	let grad = sqrt(gx * gx + gy * gy);

    let black = clamp(1.0 - length(grad) * 0.9, 0., 1.);
    let black = pow(black, mix(1.0, 2.0, iIntensity));

    let c = textureSample(iInputsTex[0], iSampler,  uv);
    let rgb = c.rgb * (mix(1.0, black, iIntensity * pow(defaultPulse, 2.)));
    return add_alpha(rgb, c.a);
}
