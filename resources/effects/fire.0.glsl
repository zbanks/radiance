// Fire from the bottom

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);

    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec3 noise_input = vec3(normCoord * 3. + vec2(0., -iTime * 0.5), iTime * 0.3);
    vec2 shift = (vec2(noise(noise_input), noise(noise_input + 100.)) - 0.5);
    shift += (vec2(noise(2. * noise_input), noise(2. * noise_input + 100.)) - 0.5) * 0.5;
    shift += (vec2(noise(4. * noise_input), noise(4. * noise_input + 100.)) - 0.5) * 0.25;
    shift = (iIntensity * 0.5 + 0.5) * shift + vec2(0., 0.5 - 0.5 * iIntensity);
    shift /= aspectCorrection;

    uv = uv + shift;
    vec4 color = vec4(1., uv.y * 0.6, 0., smoothstep(0.1, 0.3, (1. - uv.y)));
    
    color *= smoothstep(0., 0.2, iIntensity);
    
    gl_FragColor = composite(gl_FragColor, color);
}
