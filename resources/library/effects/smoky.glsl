#property description Emit smoke from the object

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv); // The smoke is stored and drawn on iChannel[1]
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(c, fragColor);
}

#buffershader

// Create a 1/r circulating vector field
// using the given matrix transform

vec2 circulate(mat3 xf) {
    // Calculate transformed point
    vec2 pt = (vec3(uv, 1.) * xf).xy;

    // Create circulating vector field
    // with the appropriate magnitude
    float l = length(pt);
    vec2 val = 0.01 * pt * mat2(0., 1., -1., 0.) / (l * l);

    // Prevent weirdness in the center
    val *= smoothstep(0., 0.1, l);
    return val;
}

// Create a compelling circulating vector field
vec2 vectorField() {
    vec2 field = vec2(0.);

    // Combine three calls to circulate()
    for (int i=0; i<3; i++) {
        vec4 n = texture2D(iNoise, vec2(0.5 + i / 20., 0.5 + iIntensityIntegral * 0.00006));

        // Random xy scaling
        float sx = 5. * (n.b - 0.5) * iIntensity;
        float sy = 5. * (n.a - 0.5) * iIntensity;

        // Random xy translation as well
        mat3 xf = mat3(sx, 0., -n.r * sx,
                       0., sy, -n.g * sy,
                       0., 0., 1.);

        // Superimpose
        field += circulate(xf);
    }

    // Give a slight bit of divergence
    field -= (uv - 0.5) * iIntensity * iIntensity * 0.3;
    return field;
}

void main(void) {
    float dt = 0.01;

    // Nudge according to vector field
    fragColor = texture(iChannel[1], uv + vectorField() * dt);

    // Fade out
    fragColor *= exp((iIntensity - 2.) / 300.);

    // Clear when intensity is zero
    fragColor *= smoothstep(0., 0.1, iIntensity);

    // Desaturate
    float avgRGB = (fragColor.r + fragColor.g + fragColor.b) / 3.;
    fragColor.rgb = mix(fragColor.rgb, vec3(avgRGB), 0.05);

    // Composite with input
    vec4 c = texture(iInput, uv);
    fragColor = composite(fragColor, c);
}
