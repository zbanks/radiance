#property description Lorenz attractor

vec3 lorenz(vec3 xyz, vec3 srb) {
    // Compute the derivative of the Lorenz system at point `xyz` given parameters `srb`
    // srb.xyz --> sigma, rho, beta
    // https://en.wikipedia.org/wiki/Lorenz_system

    return vec3(
        srb.x * (xyz.y - xyz.x),
        xyz.x * (srb.y - xyz.z) - xyz.y,
        xyz.x * xyz.y - srb.z * xyz.z);
}

void main(void) {
    // Classic Lorenz curve parameters
    vec3 srb = vec3(10., 28., 8. / 3.); // sigma, rho, beta paramaters

    // Initial coordinate
    vec3 xyz = vec3(uv - 0.5, sin(iTime * iFrequency));
    xyz *= mix(vec3(1.), vec3(90., 120., 20), smoothstep(0., 0.7, iIntensity));
    xyz += vec3(10., 0., 30.);

    #define N_STEPS 20
    #define STEP_SIZE 0.01
    #define MAGIC_SCALE 0.01 // function of other 2 values... somehow
    vec3 start = xyz;
    for (int i = 0; i < N_STEPS; i++) {
        // Rough Euler's method
        xyz += lorenz(xyz, srb) * STEP_SIZE;
    }

    // How much did wwe move?
    vec3 dxyz = (xyz - start);
    vec2 duv = (dxyz.xy + 0.5 * dxyz.z) * MAGIC_SCALE;

    // Distort along contour
    vec2 newUV = uv + duv * iIntensity;
    fragColor = texture(iInput, newUV) * box(newUV);
}
