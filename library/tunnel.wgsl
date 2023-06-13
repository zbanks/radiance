#property description 3D tunnel
//#property author https://www.shadertoy.com/view/4sXSzs
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
	let q = (uv - vec2<f32>(0.5, 0.5)) * aspectCorrection;

    let t2 = iFrequency * iTime * (pi / 16.);
    let offset = vec2<f32>(sin(t2), cos(t2)) * iFrequency * 0.1;
    let q = q - (offset);

	let len = length(q);

    let t = iFrequency * iTime;
	let a = 6. * atan2(q.y, q.x) / (2. * pi) + t * 0.3;
	let b = 6. * atan2(q.y, q.x) / (2. * pi) + t * 0.3;
	let r1 = 0.3 / len + t * 0.5;
	let r2 = 0.2 / len + t * 0.5;

    let texcoords = vec2<f32>(a + 0.1 / len, r1);
    let texcoords = abs(fract(texcoords * 0.5) * 2. - 1.);

    let fragColor = textureSample(iInputsTex[0], iSampler,  mix(uv, texcoords, iIntensity));

    return fragColor * (smoothstep(0., 0.1 * iIntensity, len));
}
