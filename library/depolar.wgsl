#property description Convert rings to vertical lines

fn main(uv: vec2<f32>) -> vec4<f32> {
    let angle  = uv.y * pi;
    let lengthFactor = 1.0; // sqrt(2.);
    let rtheta = uv.x * lengthFactor * vec2<f32>(sin(angle), -cos(angle));
    let rtheta = rtheta / aspectCorrection;
    let rtheta = (rtheta + 1.) / 2.;

    let uv2 = mix(uv, rtheta, iIntensity * pow(defaultPulse, 2.));

    return textureSample(iInputsTex[0], iSampler, uv2);
}
