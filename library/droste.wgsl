//#property description Droste effect (spiral forever!)
//#property author http://roy.red/droste-.html + zbanks
#property frequency 1

// Complex functions from http://glslsandbox.com/e#5664.1

fn cconj(c: vec2<f32>) -> vec2<f32> { return vec2<f32>(c.x, -c.y); }

fn cmul(c1: vec2<f32>, c2: vec2<f32>) -> vec2<f32> {
	return vec2<f32>(
		c1.x * c2.x - c1.y * c2.y,
		c1.x * c2.y + c1.y * c2.x
	);
}

fn cdiv(c1: vec2<f32>, c2: vec2<f32>) -> vec2<f32> {
	return cmul(c1, cconj(c2)) / dot(c2, c2);
}

fn clog(z: vec2<f32>) -> vec2<f32> {
	return vec2<f32> (log(length(z)), atan2(z.y, z.x));
}

fn circle(a: f32) -> vec2<f32> { return vec2<f32>(cos(a), sin(a)); }

fn cexp(z: vec2<f32>) -> vec2<f32> {
	return circle(z.y) * exp(z.x);
}

const r1: f32 = 0.1;
const r2: f32 = 2.0;

fn droste(z: vec2<f32>) -> vec2<f32> {
    // 4. Take the tiled strips back to ordinary space.
    let z2 = clog(z);
    // 3. Scale and rotate the strips
    let scale = log(r2/r1);
    // Negate the angle to twist the other way
    let angle = atan(scale/(2.0*pi));
    var z3 = cdiv(z2, cexp(vec2(0.0,angle))*cos(angle)); 
    // 2. Tile the strips
    z3.x -= iTime * iFrequency * scale * 0.25;
    z3.x = modulo(z3.x, scale);
    // 1. Take the annulus to a strip
    let z4 = cexp(z3)*r1;
    let z5 = z4 / (r2 * 2.0);
    return z5;
}

fn f(x: f32, n: f32) -> f32{
    return pow(n,-floor(log(x)/log(n)));
}

//vec2 droste2(vec2 z) {
//    float ratio = 5.264;
//    float angle = atan(log(ratio)/(2.0*pi));
//    z = cexp(cdiv(clog(z), cexp(vec2(0,angle))*cos(angle)));
//    vec2 a_z = abs(z);
//    z *= f(max(a_z.x,a_z.y)*2.,ratio);
//    return z / ratio;
//}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5) * aspectCorrection;
    let newUV1 = droste(normCoord);
    let newUV2 = newUV1 / aspectCorrection + 0.5;

    return textureSample(iInputsTex[0], iSampler, mix(uv, newUV2, iIntensity));
}
