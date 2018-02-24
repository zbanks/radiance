#property description Flow the first image around the second using lightness gradient
#property inputCount 2

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}

#buffershader

vec2 getGradient() {
    vec2 EPSILON = vec2(0.01);

    vec4 val = texture(iInputs[1], uv);

    // Take a small step in X
    vec4 dcdx = (texture(iInputs[1], uv + vec2(EPSILON.x, 0.)) - val) / EPSILON.x;

    // Take a small step in Y
    vec4 dcdy = (texture(iInputs[1], uv + vec2(0., EPSILON.y)) - val) / EPSILON.y;

    vec2 dc = vec2(dot(dcdx.rgb, vec3(1.)), dot(dcdy.rgb, vec3(1.)));

    dc = clamp(0.008 * dc, -1., 1.);

    return dc;
}

void main(void) {
    vec4 c1 = texture(iInputs[0], uv);

    // Perturb according to gradient
    vec2 perturb = -getGradient(); // Avoid dark
    vec4 c2 = texture(iChannel[1], uv + 0.05 * iIntensity * perturb);

    // Blend between the current frame and a slightly shifted down version of it using the max function
    fragColor = max(c1, c2);

    // Fade out according to the beat
    fragColor *= pow(defaultPulse, 0.3);

    // Fade out slowly
    float fadeAmount = 0.01 + 0.2 * (1. - iIntensity);
    fragColor = max(fragColor - fadeAmount, vec4(0.));

    // Clear back buffer when intensity is low
    fragColor *= smoothstep(0., 0.1, iIntensity);
}
