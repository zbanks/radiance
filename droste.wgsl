//#property description Droste effect (spiral forever!)
//#property author http://roy.red/droste-.html + zbanks
//#property frequency 1

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

let r1: f32 = 0.1;
let r2: f32 = 2.0;

fn droste(z: vec2<f32>) -> vec2<f32> {
    // 4. Take the tiled strips back to ordinary space.
    let z = clog(z);
    // 3. Scale and rotate the strips
    let scale = log(r2/r1);
    // Negate the angle to twist the other way
    let angle = atan(scale/(2.0*pi));
    var z = cdiv(z, cexp(vec2(0.0,angle))*cos(angle)); 
    // 2. Tile the strips
    z.x -= iTime() * iFrequency();
    z.x = z.x % scale;
    // 1. Take the annulus to a strip
    let z = cexp(z)*r1;
    let z = z / (r2 * 2.0);
    return z;
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
    let normCoord = 2. * (uv - 0.5) * aspectCorrection();
    let newUV = droste(normCoord);
    let newUV = newUV / aspectCorrection() + 0.5;

    return textureSample(iInputsTex[0], iSampler, mix(uv, newUV, iIntensity()));
}
