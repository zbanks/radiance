#property description Emboss first input according to second
#property inputCount 2

// Return the height for a given uv point
float height(vec2 pos) {
    vec4 c = texture(iInputs[1], pos);
    float amt = (c.r + c.g + c.b) / 3.;
    return (amt - 1.) * c.a * 0.03 * iIntensity;
}

// Estimate the normal of the new displacement-mapped surface
vec3 estimateNormal() {
    vec3 pt = vec3(uv, height(uv));

    vec2 EPSILON = 1. / iResolution; // One pixel

    // Take a small step in X
    vec3 stepX = vec3(EPSILON.x, 0., 0.);
    stepX.z = height(uv + stepX.xy);
    stepX = stepX - pt;

    // Take a small step in Y
    vec3 stepY = vec3(0., EPSILON.y, 0.);
    stepY.z = height(uv + stepY.xy);
    stepY = stepY - pt;

    // Return the cross product of these vectors
    // to get the surface normal
    return normalize(-cross(stepX, stepY));
}

void main(void) {
    // Look up the color of the point
    // (iInputs[0] displaced by a small amount up according to height)
    vec4 c = texture(iInputs[0], uv + vec2(0., 0.5 * height(uv)));

    // Specular exponent
    float shininess = 40.;

    // Specular amount
    float k_spec = iIntensity * 0.5;

    // Diffuse amount
    float k_diff = 0.8;

    // Ambient amount
    float k_amb = 0.1;

    // Image is in the Z-plane spanning 0,0 to 1,1
    vec3 pt = vec3(uv, 0.);
    vec3 viewer = vec3(0.5, -2., 4.);
    vec3 light = vec3(0.5, 2., 1.);

    // Blinn-Phong model
    vec3 n = estimateNormal();
    vec3 l = normalize(light - pt);
    vec3 v = normalize(viewer - pt);
    vec3 h = normalize(l + v); // Halfway vector
    float spec = pow(dot(n, h), shininess) * k_spec; // Specular component
    float diff = max(dot(n, l), 0.) * k_diff; // Diffuse component

    // Compute the shaded color of this pixel
    vec4 shaded = min(c * k_amb + c * diff * k_diff + spec, 1.);

    // Blend between the flat-shaded and phong-shaded colors
    // to preserve identity
    fragColor = mix(c, shaded, smoothstep(0., 0.2, iIntensity));
}
