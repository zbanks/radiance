#property description Loopy laser-like pattern

void main(void) {
    vec4 originalColor = texture(iInput, uv);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    // Decompose coordinate into R-theta
    float r = length(normCoord);
    float angle = atan(normCoord.x, normCoord.y);

    // Compute the size/position of the 'bulge' that travels around the circle
    float BULGE_SPEED = 1. / 8.0;
    float BULGE_WIDTH = mix(0.20, 0.28, iAudioLevel);  // 0.25
    float BULGE_HEIGHT = mix(0.01, 0.035, iIntensity); // 0.03
    float NOBULGE_HEIGHT = 0.01;
    float angle_t = mod(angle / M_PI + iTime * BULGE_SPEED, 2.0) - 1.0;
    float bulge = exp(-pow(angle_t / BULGE_WIDTH, 2.)) * BULGE_HEIGHT;
    bulge += NOBULGE_HEIGHT;

    // Compute distance from circle, including bulge & standing wave
    float CIRCLE_R = mix(0.10, 0.40, iIntensity); //0.33
    float WAVE_COUNT = 16.;
    float WAVE_SPEED = 1 / 4.0;
    float d = abs(r - CIRCLE_R - bulge * sin(angle * WAVE_COUNT - iTime * M_PI * WAVE_SPEED));

    // Turn distance into alpha 
    float THICKNESS = mix(0.07, 0.12, iAudioLow); // 0.10
    float alpha = max(0.0, 1.0 - 10. * d);
    alpha = max(0., cos(min(1.8, (1.0 - alpha) / THICKNESS)));

    // Composite purple onto input
    vec4 color = vec4(0.8, 0.1, 0.9, 1.0);
    color *= alpha;
    color *= smoothstep(0.0, 0.1, iIntensity);
    fragColor = composite(originalColor, color);
}
