#property description Loopy laser-like pattern
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let originalColor = textureSample(iInputsTex[0], iSampler,  uv);
    let normCoord = (uv - 0.5) * aspectCorrection;

    // Decompose coordinate into R-theta
    let r = length(normCoord);
    let angle = atan2(normCoord.x, normCoord.y);

    // Compute the size/position of the 'bulge' that travels around the circle
    let BULGE_SPEED = 1.;
    let BULGE_WIDTH = mix(0.20, 0.28, iAudioLevel);  // 0.25
    let BULGE_HEIGHT = mix(0.01, 0.035, iIntensity); // 0.03
    let NOBULGE_HEIGHT = 0.01;
    let angle_t = (2. + angle / pi + iTime * iFrequency * BULGE_SPEED) % 2.0 - 1.0;
    let bulge = exp(-pow(angle_t / BULGE_WIDTH, 2.)) * BULGE_HEIGHT;
    let bulge = bulge + (NOBULGE_HEIGHT);

    // Compute distance from circle, including bulge & standing wave
    let CIRCLE_R = mix(0.10, 0.40, iIntensity); //0.33
    let WAVE_COUNT = 16.;
    let WAVE_SPEED = 1. / 4.0;
    let d = abs(r - CIRCLE_R - bulge * sin(angle * WAVE_COUNT - iTime * pi * WAVE_SPEED));

    // Turn distance into alpha 
    let THICKNESS = mix(0.07, 0.12, iAudioLow); // 0.10
    let alpha = max(0.0, 1.0 - 10. * d);
    let alpha = max(0., cos(min(1.8, (1.0 - alpha) / THICKNESS)));

    // Composite purple onto input
    let color = vec4<f32>(0.8, 0.1, 0.9, 1.0);
    let color = color * (alpha);
    let color = color * (smoothstep(0.0, 0.1, iIntensity));
    return composite(originalColor, color);
}
