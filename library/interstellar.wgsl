#property description Lightspeed travel rays
//#property author https://www.shadertoy.com/view/Xdl3D2
#property frequency 1

let tau = 6.28318530717958647692;

// Gamma correction
let GAMMA = 2.2;

fn ToLinear(col: vec3<f32>) -> vec3<f32> {
	// simulate a monitor, converting colour values into light values
	return pow( col, vec3<f32>(GAMMA) );
}

fn ToGamma( col: vec3<f32> ) -> vec3<f32> {
	// convert back into colour values, so the correct light will come out of the monitor
	return pow( col, vec3<f32>(1.0/GAMMA) );
}

fn Noise(x: vec2<i32>) -> vec4<f32> {
	return 2. * textureSample(iNoiseTex, iSampler, (vec2<f32>(x) + 0.5) / 256.0 + 0.5);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
	//let ray_xy = 2.0*(gl_FragCoord.xy-iResolution.xy*.5)/iResolution.x;
	let ray_xy = (uv - 0.5) * aspectCorrection;
    let ray = vec3<f32>(ray_xy, 1.);

	//let offset = iTime*.5;
	//let speed2 = (cos(offset)+1.0)*2.0;
	let offset = iTime * iFrequency * 0.25;
	let speed2 = 3. * iAudioLow;
	let speed = speed2+.1;
	//let offset = //offset + (sin(offset)*.96);
	//let offset = //offset * (2.0);

	var col = vec3<f32>(0.);

	let stp = ray/max(abs(ray.x),abs(ray.y));

	var pos = 2.0 * stp + 0.5;
	for ( var i=0; i < 20; i++ )
	{
		let z = Noise(vec2<i32>(pos.xy)).x;
		let z = fract(z - offset);
		let d = 50.0 * z - pos.z;
		let w = pow(max(0.0, 1.0 - 8.0 * length(fract(pos.xy) - .5)), 2.0);
		let c = max(
            vec3<f32>(0.),
            vec3<f32>(1.0 - abs(d + speed2 * .5) / speed,
                      1.0 - abs(d) / speed,
                      1.0 - abs(d - speed2 * .5) / speed
            )
        );
		col += 1.5 * (1.0 - z) * w * c;
		pos += stp;
	}

    let fc_rgb = ToGamma(col);
    let fc_a = max(max(fc_rgb.r, fc_rgb.g), fc_rgb.b);
	let fc = vec4<f32>(fc_rgb, fc_a) * smoothstep(0., 0.2, iIntensity);

    let c = textureSample(iInputsTex[0], iSampler,  uv);
    return max(c, fc);
}
