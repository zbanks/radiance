#property description Excessive rainbow
#property frequency 1

// A radiance classic

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = vec4(hsv2rgb(vec3(mod(uv.x + iTime * iFrequency * 0.5, 1.), 1., 1.)), 1.);
    fragColor = mix(fragColor, c, iIntensity);
}
