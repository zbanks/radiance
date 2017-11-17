#property description Mirror and repeat the pattern in a circle

void main(void) {
    vec2 normCoord = 2. * (uv - 0.5);
    normCoord *= aspectCorrection;
    float r = length(normCoord);
    float theta = atan(normCoord.y, normCoord.x);

    float bins = iIntensity * 5. + 2.;
    float tStep = M_PI / bins;
    theta = abs(mod(theta + tStep, 2. * tStep) - tStep);

    vec2 newUV = r * vec2(cos(theta), sin(theta));
    newUV *= 0.707;
    newUV /= aspectCorrection;
    newUV = newUV * 0.5 + 0.5;

    fragColor = texture(iInput, mix(uv, newUV, smoothstep(0., 0.2, iIntensity)));
}
