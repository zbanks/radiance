// Fileball in the center

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    vec3 noise_input = vec3(length(uv - vec2(0.5)) * 3. - iTime, abs(atan(uv.y - 0.5, uv.x - 0.5)), iTime * 0.3);
    vec2 shift = (vec2(noise(noise_input), noise(noise_input + 100.)) - 0.5);
    shift += (vec2(noise(2. * noise_input), noise(2. * noise_input + 100.)) - 0.5) * 0.5;
    shift += (vec2(noise(4. * noise_input), noise(4. * noise_input + 100.)) - 0.5) * 0.25;
    shift = (iIntensity * 0.5 + 0.1) * shift;

    uv = uv + shift;
    vec4 color = vec4(1., length(uv - vec2(0.5)) * 2., 0., smoothstep(0.5, 0.7, (1. - length(uv - vec2(0.5)))));
    
    color.a *= smoothstep(0., 0.2, iIntensity);
    
    gl_FragColor = composite(gl_FragColor, color);
}
