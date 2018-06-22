#property description Solid colors folowing resistor code

void main(void) {
    vec3 c = vec3(1.0);
    if (iIntensity < 0.09) c = vec3(0.0, 0.0, 0.0);
    else if (iIntensity < 0.19) c = vec3(0.4, 0.2, 0.2);
    else if (iIntensity < 0.29) c = vec3(1.0, 0.0, 0.0);
    else if (iIntensity < 0.39) c = vec3(1.0, 0.5, 0.0);
    else if (iIntensity < 0.49) c = vec3(1.0, 1.0, 0.0);
    else if (iIntensity < 0.59) c = vec3(0.0, 1.0, 0.0);
    else if (iIntensity < 0.69) c = vec3(0.0, 0.0, 1.0);
    else if (iIntensity < 0.79) c = vec3(0.5, 0.0, 0.9);
    else if (iIntensity < 0.89) c = vec3(0.5, 0.5, 0.6);

    fragColor = texture(iInput, uv);
    fragColor = mix(fragColor, vec4(c, 1.0), smoothstep(0.0, 0.1, iIntensity));
}
