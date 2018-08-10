#property description Convert to grayscale/greyscale/monochrome/black & white

void main(void) {
    vec4 original = texture(iInput, uv);
    float y = dot(vec3(0.2627, 0.6780, 0.0593), original.rgb);
    vec4 bw = vec4(y, y, y, original.a);
    fragColor = mix(original, bw, iIntensity * pow(defaultPulse, 2.0));
}
