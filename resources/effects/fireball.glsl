// Fileball in the center

void main(void) {
    gl_FragColor = texture2D(iInput, uv);

    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec3 noise_input = vec3(length(normCoord) * 3. - iTime, abs(atan(normCoord.y, normCoord.x)), iTime * 0.3);
    vec2 shift = (vec2(noise(noise_input), noise(noise_input + 100.)) - 0.5);
    shift += (vec2(noise(2. * noise_input), noise(2. * noise_input + 100.)) - 0.5) * 0.5;
    shift += (vec2(noise(4. * noise_input), noise(4. * noise_input + 100.)) - 0.5) * 0.25;
    shift = (iIntensity * 0.7 + 0.3) * shift;

    normCoord = normCoord + shift;
    vec4 color = vec4(1., length(normCoord) * 2., 0., 1.0);
    color *= smoothstep(0.4, 0.5, (1. - length(normCoord)));
    color *= smoothstep(0., 0.2, iIntensity);

    gl_FragColor = composite(gl_FragColor, color);
}
