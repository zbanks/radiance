// Change the color (in HSV) to red

void main(void) {
    vec4 c = texture2D(iInput, uv);
    gl_FragColor.r = mix(c.r, (c.r + c.g + c.b) / 3., iIntensity);
    gl_FragColor.g = c.g * (1. - iIntensity);
    gl_FragColor.b = c.b * (1. - iIntensity);
    gl_FragColor.a = c.a;
    gl_FragColor = clamp(gl_FragColor, 0.0, 1.0);
}
