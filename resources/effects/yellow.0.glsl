// Yellow and green vertical waves

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    gl_FragColor = texture2D(iInput, uv);

    float y = smoothstep(0.2, 0.7, abs(mod(normCoord.x * 4. - iIntensityIntegral, 2.) - 1.));
    float g = smoothstep(0.5, 0.9, abs(mod(1. + normCoord.x * 4. - iIntensityIntegral, 2.) - 1.));

    vec4 c = vec4(1., 1., 0., y);
    c = composite(c, vec4(0., 1., 0., g * smoothstep(0.5, 0.8, iIntensity)));

    c.a *= smoothstep(0., 0.1, iIntensity);
    c = clamp(c, 0., 1.);
    gl_FragColor = composite(gl_FragColor, premultiply(c));
}
