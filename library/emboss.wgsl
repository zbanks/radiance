#property description Emboss first input according to second
#property inputCount 2

// Return the height for a given uv point
fn height(uv: vec2<f32>) -> f32 {
    let c = textureSample(iInputsTex[1], iSampler,  uv);
    let amt = (c.r + c.g + c.b) / 3.;
    return (amt - 1.) * c.a * 0.03 * iIntensity * pow(defaultPulse, 2.);
}

// Estimate the normal of the new displacement-mapped surface
fn estimateNormal(uv: vec2<f32>) -> vec3<f32> {
    let pt = vec3<f32>(uv, height(uv));

    // Take a small step in X
    let ptStepX = uv + vec2<f32>(onePixel.x, 0.);
    let z = height(ptStepX);
    let stepX = vec3<f32>(ptStepX, z) - pt;

    // Take a small step in Y
    let ptStepY = uv + vec2<f32>(0., onePixel.y);
    let z = height(ptStepY);
    let stepY = vec3<f32>(ptStepY, z) - pt;

    // Return the cross product of these vectors
    // to get the surface normal
    return normalize(-cross(stepX, stepY));
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Look up the color of the point
    // (iInputs[0] displaced by a small amount up according to height)
    let c = textureSample(iInputsTex[0], iSampler,  uv + vec2<f32>(0., 0.5 * height(uv)));

    // Specular exponent
    let shininess = 100.;

    // Specular amount
    let k_spec = iIntensity * 0.3;

    // Diffuse amount
    let k_diff = 1.3;

    // Ambient amount
    let k_amb = 0.3;

    // Image is in the Z-plane spanning 0,0 to 1,1
    let pt = vec3<f32>(uv, 0.);
    let viewer = vec3<f32>(0.5, -2., 4.);
    let light = vec3<f32>(0.5, 2., 1.);

    // Blinn-Phong model
    let n = estimateNormal(uv);
    let l = normalize(light - pt);
    let v = normalize(viewer - pt);
    let h = normalize(l + v); // Halfway vector
    let spec = pow(dot(n, h), shininess) * k_spec; // Specular component
    let diff = max(dot(n, l), 0.) * k_diff; // Diffuse component

    // Compute the shaded color of this pixel
    let shaded = min(c * k_amb + c * diff * k_diff + spec, vec4<f32>(1.));

    // Blend between the flat-shaded and phong-shaded colors
    // to preserve identity
    return mix(c, shaded, smoothstep(0., 0.2, iIntensity));
}
