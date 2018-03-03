#property description Like you're looking at the image reflected in something that was surfaced on a lathe
#property frequency 0.25

void main(void) {
    vec2 normCoord = 2. * (uv - 0.5);
    normCoord *= aspectCorrection;

    // Look up r and theta
    float r = length(normCoord);
    float theta = atan(normCoord.y, normCoord.x);

    // Image is sampled along a vertical line that slowly shifts back and forth
    // and this line is then swept into a circle
    // Get two newUVs, one for the top half and one for the bottom half
    vec2 newUV1 = vec2(sin(iTime * iFrequency * 0.5 * M_PI), r);
    vec2 newUV2 = vec2(sin(iTime * iFrequency * 0.5 * M_PI), -r);
    newUV1 /= aspectCorrection;
    newUV1 = newUV1 * 0.5 + 0.5;
    newUV2 /= aspectCorrection;
    newUV2 = newUV2 * 0.5 + 0.5;

    // Make them both converge to the old UV at low intensity so that identity holds
    newUV1 = mix(uv, newUV1, smoothstep(0., 0.3, iIntensity));
    newUV2 = mix(uv, newUV2, smoothstep(0., 0.3, iIntensity));

    vec4 c1 = texture(iInput, newUV1);
    vec4 c2 = texture(iInput, newUV2);

    // Mix them based on angle
    fragColor = mix(c2, c1, smoothstep(-0.5, 0.5, sin(theta)));
}
