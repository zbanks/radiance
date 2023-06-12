#property description Emit smoke from the object

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = textureSample(iChannelsTex[1], iSampler,  uv); // The smoke is stored and drawn on iChannelsTex[1]
    let c = c * (smoothstep(0., 0.2, iIntensity));
    let fragColor = composite(c, fragColor);
    return fragColor;
}

#buffershader

// Create a 1/r circulating vector field
// using the given matrix transform

fn circulate(uv: vec2<f32>, xf: mat3x3<f32>) -> vec2<f32> {
    // Calculate transformed point
    let pt = (vec3<f32>(uv, 1.) * xf).xy;

    // Create circulating vector field
    // with the appropriate magnitude
    let l = length(pt);
    let val = 0.01 * pt * mat2x2<f32>(0., 1., -1., 0.) / (l * l);

    // Prevent weirdness in the center
    let val = val * (smoothstep(0., 0.1, l));
    return val;
}

// Create a compelling circulating vector field
fn vectorField(uv: vec2<f32>) -> vec2<f32> {
    var field = vec2<f32>(0.);

    // Combine three calls to circulate()
    for (var i=0; i<3; i++) {
        let n = textureSample(iNoiseTex, iSampler, vec2<f32>(0.5 + f32(i) / 20., 0.5 + iIntensityIntegral * 0.00006));

        // Random xy scaling
        let sx = 5. * (n.b - 0.5) * iIntensity;
        let sy = 5. * (n.a - 0.5) * iIntensity;

        // Random xy translation as well
        let xf = mat3x3<f32>(sx, 0., -n.r * sx,
                             0., sy, -n.g * sy,
                             0., 0., 1.);

        // Superimpose
        field += circulate(uv, xf);
    }

    // Give a slight bit of divergence
    let field = field - ((uv - 0.5) * iIntensity * iIntensity * 0.3);
    return field;
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    //let dt = 0.01 + iFrequency / 256.0;
    let dt = iStep * (0.25 + iFrequency * 4.);

    let fadeout = iStep * 5.;

    // Nudge according to vector field
    let fragColor = textureSample(iChannelsTex[1], iSampler,  uv + vectorField(uv) * dt);

    // Fade out
    let fragColor = fragColor * exp((iIntensity - 2.) * fadeout / 3.);

    // Clear when intensity is zero
    let fragColor = fragColor * smoothstep(0., 0.1, iIntensity);

    // Desaturate
    let avgRGB = (fragColor.r + fragColor.g + fragColor.b) / 3.;
    let rgb = mix(fragColor.rgb, vec3<f32>(avgRGB), 0.05);
    let fragColor = vec4<f32>(rgb, fragColor.a);

    // Composite with input
    let c = textureSample(iInputsTex[0], iSampler,  uv);
    return composite(fragColor, c);
}
