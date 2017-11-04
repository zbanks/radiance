#property description And the cracks begin to show

void main(void) {
    // Number of cracks dependent on intensity
    int numCracks = int(iIntensity * 15.);

    // Timebase for time-dependent cracking
    float t = iTime * 0.5;

    // Amount to perturb the input
    vec2 perturb = vec2(0.);

    // Becomes 1 when we are close to a crack line
    float line = 0.;

    for (int i=0; i<numCracks; i++) {
        // Store a vec4 of parameters for each crack
        // .xy is the center point
        // .zw is the direction vector
        vec4 crackParameters = texture(iNoise, vec2(i, floor(t)) * onePixel * aspectCorrection);
        crackParameters.zw = crackParameters.zw - 0.5;
        crackParameters.zw /= length(crackParameters.zw);

        // Find the vector normal to the crack
        vec2 normalVector = crackParameters.wz * vec2(-1., 1.);

        // Find which side of the crack we are on
        float side = dot(uv - crackParameters.xy, normalVector);

        // Perturb the input towards the crack
        // (as if a mirror were shattered by hitting it)
        float perturbAmt = step(side, 0.) - 0.5;
        vec2 perturbDir = normalVector;
        perturb += perturbDir * perturbAmt;

        // If the dot product is close to zero, we are on the crack line
        line += 1. - smoothstep(0., onePixel, abs(side));
    }
    line = min(line, 1.);

    // Don't perturb off the edges
    perturb *= 1. - 2. * abs(uv - 0.5);

    // Perturb to the beat
    perturb *= sawtooth(t, 0.01);

    // Perturb proprtional to intensity, but not on crack lines
    fragColor = texture(iInput, uv - perturb * 0.1 * (1.2 - iIntensity) * (1. - line));
}
