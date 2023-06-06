#property description Lorenz attractor

fn lorenz(xyz: vec3<f32>, srb: vec3<f32>) -> vec3<f32> {
    // Compute the derivative of the Lorenz system at point `xyz` given parameters `srb`
    // srb.xyz --> sigma, rho, beta
    // https://en.wikipedia.org/wiki/Lorenz_system

    return vec3<f32>(
        srb.x * (xyz.y - xyz.x),
        xyz.x * (srb.y - xyz.z) - xyz.y,
        xyz.x * xyz.y - srb.z * xyz.z);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Classic Lorenz curve parameters
    let srb = vec3<f32>(10., 28., 8. / 3.); // sigma, rho, beta paramaters

    // Initial coordinate
    var xyz = vec3<f32>(uv - 0.5, sin(iTime * iFrequency));
    xyz *= mix(vec3<f32>(1.), vec3<f32>(90., 120., 20.), smoothstep(0., 0.7, iIntensity));
    xyz += vec3<f32>(10., 0., 30.);

    let N_STEPS = 20;
    let STEP_SIZE = 0.01;
    let MAGIC_SCALE = 0.01; // function of other 2 values... somehow
    let start = xyz;
    for (var i = 0; i < N_STEPS; i++) {
        // Rough Euler's method
        xyz += lorenz(xyz, srb) * STEP_SIZE;
    }

    // How much did we move?
    let dxyz = (xyz - start);
    let duv = (dxyz.xy + 0.5 * dxyz.z) * MAGIC_SCALE;

    // Distort along contour
    let newUV = uv + duv * iIntensity;
    return textureSample(iInputsTex[0], iSampler,  newUV) * box(newUV);
}
