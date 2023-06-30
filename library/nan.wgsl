#property description Fill frame with NANs or INFs

fn main(uv: vec2<f32>) -> vec4<f32> {
    let nan = 0. / 0.;
    let n_inf = -1. / 0.;
    let p_inf = 1. / 0.;

    if iIntensity < 0.25 {
        return textureSampleLevel(iInputsTex[0], iSampler, uv, 0.);
    } else if iIntensity < 0.5 {
	return vec4<f32>(n_inf);
    } else if iIntensity < 0.75 {
	return vec4<f32>(p_inf);
    } else {
	return vec4<f32>(nan);
    }
}
