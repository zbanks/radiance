#property description Game of life?

void main(void) {
    float alive = texture(iChannel[1], uv).r;
    vec4 under = texture(iInput, uv);
    vec4 over = alive * texture(iChannel[2], uv);
    over *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(under, over);
}

#buffershader

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    //float bs = 2048. * pow(2, -5. * iIntensity);
    float bs = max((1. - iIntensity) / (1.3 * onePixel), 5.);
    vec2 bins = bs * aspectCorrection;
    vec2 db = 1. / (bins * aspectCorrection);
    normCoord = round(normCoord * bins) * db + 0.5;

    float n = 0.;
    n += texture(iChannel[1], normCoord + db * vec2(-1, -1)).r;
    n += texture(iChannel[1], normCoord + db * vec2(-1,  0)).r;
    n += texture(iChannel[1], normCoord + db * vec2(-1,  1)).r;
    n += texture(iChannel[1], normCoord + db * vec2( 0, -1)).r;
    n += texture(iChannel[1], normCoord + db * vec2( 0,  1)).r;
    n += texture(iChannel[1], normCoord + db * vec2( 1, -1)).r;
    n += texture(iChannel[1], normCoord + db * vec2( 1,  0)).r;
    n += texture(iChannel[1], normCoord + db * vec2( 1,  1)).r;
    float s = texture(iChannel[1], normCoord).r;


    // Use bright areas of the source image to help "birth" pixels (or kill)
    vec4 source = texture(iInput, normCoord) * pow(defaultPulse, 2.);
    //float r = 20. * rand(vec3(normCoord, iTime)) + mix(4.0, 0, iIntensity);
    //float bonus = step(20.5, r + max(max(source.r, source.g), source.b));
    //n += bonus * 3;

    // if (s == 0) { alive = (n == 3) }
    // else { alive = (2 <= n <= 3) }
    float alive = step(1.8, n) * step(n, 3.2) * step(2.8, n + s);
    alive *= step(0.05, iIntensity); // to reset

    // Make there be life if there is sufficient input color
    //float lifeFromInput = step(0.5, smoothstep(0., 3., dot(vec3(1.), source.rgb)));
    float lifeFromInput = step(0.8, max(source.r, max(source.g, source.b)));
    alive = max(alive, lifeFromInput);
    alive *= step(0.01, texture(iChannel[2], normCoord).a); // Kill stable life if there is no color

    fragColor.gba = vec3(1.0);
    fragColor.r = alive;
}

#buffershader

// This buffer just paints the world so that life can extend
// outside of what currently has color

void main(void) {
    vec4 oldC = texture(iChannel[2], uv);
    float d = mix(0.01, 0.001, iIntensity);
    oldC = max(oldC - d, vec4(0.));
    vec4 newC = texture(iInput, uv);
    fragColor = max(oldC, newC);
}
