//#property description Organic purple waves
//#property frequency 1

void main(void) {
    vec2 aspectCorrection = vec2(1.);
    float iTime = 0.5;
    float iFrequency = 1.;
    float iIntensity = 1.;

    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    //mat2 rot = mat2(cos(iTime), -sin(iTime), sin(iTime), cos(iTime));
    vec4 c;

    float parameter = iTime * iFrequency;

    normCoord += 0.5;
    float y = pow(sin(cos(parameter / 4.) * normCoord.y * 8. + normCoord.x), 2.);
    float x = mod(sin(normCoord.x * 4.) + cos(normCoord.y * normCoord.x * 5.) * (y * 0.2 + 0.8) + 3.0, 1.0);

    c.r = mix(x, y, 0.3);
    c.b = pow(mix(x, y, 0.7), 0.6);
    c.g = 0.;
    c.a = 1.;
    c *= iIntensity;

    //fragColor = composite(texture(iInput, uv), c);
    fragColor = c;
}
