// Organic purple waves

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    //mat2 rot = mat2(cos(iTime), -sin(iTime), sin(iTime), cos(iTime));
    vec4 c;

    normCoord += 0.5;
    float y = pow(sin(cos(iTime / 4) * normCoord.y * 8 + normCoord.x), 2);
    float x = mod(sin(normCoord.x * 4) + cos(normCoord.y * normCoord.x * 5) * (y * 0.2 + 0.8) + 3.0, 1.0);

    c.r = mix(x, y, 0.3);
    c.b = pow(mix(x, y, 0.7), 0.6);
    c.g = 0;
    c.a = iIntensity;

    gl_FragColor = composite(texture2D(iFrame, uv), c);
}
