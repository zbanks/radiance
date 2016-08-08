// Fire from the bottom

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);

    vec3 noise_input = vec3(v_uv * 3. + vec2(0., -iTime * 0.3), iTime * 0.1);
    vec2 shift = (vec2(noise(noise_input), noise(noise_input + 100.)) - 0.5);
    shift += (vec2(noise(2. * noise_input), noise(2. * noise_input + 100.)) - 0.5) * 0.5;
    shift += (vec2(noise(4. * noise_input), noise(4. * noise_input + 100.)) - 0.5) * 0.25;
    shift = (iIntensity * 0.5 + 0.5) * shift + vec2(0., 0.5 - 0.5 * iIntensity);

    vec2 uv = v_uv + shift;
    vec4 color = vec4(1., uv.y * 0.6, 0., smoothstep(0.2, 0.5, (1. - uv.y)));
    
    color.a *= smoothstep(0., 0.2, iIntensity);
    
    f_color0 = composite(f_color0, color);
}
