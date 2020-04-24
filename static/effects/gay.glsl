#property description Excessive rainbow

// A radiance classic

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = vec4(hsv2rgb(vec3(mod(uv.x + iIntensityIntegral, 1.), 1., 1.)), 1.);
    fragColor = mix(fragColor, c, smoothstep(0., 0.2, iIntensity));
}
