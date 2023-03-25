#property description And the cracks begin to show
#property frequency 0.5

let MAX_CRACKS = 15;

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Number of cracks dependent on intensity
    let numCracks = i32(iIntensity * f32(MAX_CRACKS));

    // Timebase for time-dependent cracking
    let t = iTime * iFrequency;

    // Amount to perturb the input
    var perturb = vec2<f32>(0.);

    // Becomes 1 when we are close to a crack line
    var line_ = 0.;

    let crackWidth = length(onePixel);

    for (var i: i32 = 0; i<MAX_CRACKS; i++) {
        if i >= numCracks {
            break;
        }

        // Store a vec4 of parameters for each crack
        // .xy is the center point
        // .zw is the direction vector
        let crackParameters = textureSample(iNoiseTex, iSampler, vec2(f32(i) + floor(t)) * onePixel * aspectCorrection);
        let crackParametersZw = crackParameters.zw - 0.5;
        let crackParametersZw = crackParametersZw / length(crackParametersZw);
        let crackParameters = vec4<f32>(crackParameters.xy, crackParametersZw);

        // Find the vector normal to the crack
        let normalVector = crackParameters.wz * vec2(-1., 1.);

        // Find which side of the crack we are on
        let side = dot(uv - crackParameters.xy, normalVector);

        // Perturb the input towards the crack
        // (as if a mirror were shattered by hitting it)
        let perturbAmt = step(side, 0.) - 0.5;
        let perturbDir = normalVector;
        perturb += perturbDir * perturbAmt;

        // If the dot product is close to zero, we are on the crack line
        line_ += 1. - smoothstep(0., crackWidth, abs(side));
    }
    line_ = min(line_, 1.);

    // Don't perturb off the edges
    perturb *= 1. - 2. * abs(uv - 0.5);

    // Perturb to the beat
    //perturb *= sawtooth(t, 0.01);

    // Perturb proprtional to intensity, but not on crack lines
    return textureSample(iInputsTex[0], iSampler, uv - perturb * 0.1 * (1.2 - iIntensity) * (1. - line_));
}
