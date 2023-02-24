#property description Flow the first image around the second using lightness gradient
#property inputCount 2

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iChannelsTex[1], iSampler, uv);
    let c = c * smoothstep(0., 0.2, iIntensity);
    return composite(textureSample(iInputsTex[0], iSampler, uv), c);
}

#buffershader

fn getGradient(uv: vec2<f32>) -> vec2<f32> {
    let EPSILON = vec2<f32>(0.01);

    let val = textureSample(iInputsTex[1], iSampler, uv);

    // Take a small step in X
    let dcdx = (textureSample(iInputsTex[1], iSampler, uv + vec2(EPSILON.x, 0.)) - val) / EPSILON.x;

    // Take a small step in Y
    let dcdy = (textureSample(iInputsTex[1], iSampler, uv + vec2(0., EPSILON.y)) - val) / EPSILON.y;

    let dc = vec2<f32>(dot(dcdx.rgb, vec3<f32>(1.)), dot(dcdy.rgb, vec3<f32>(1.)));
    let dc = clamp(0.008 * dc, vec2<f32>(-1.), vec2<f32>(1.));

    return dc;
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c1 = textureSample(iInputsTex[0], iSampler, uv);

    // Perturb according to gradient
    let perturb = -getGradient(uv); // Avoid dark
    let c2 = textureSample(iChannelsTex[1], iSampler, uv + 0.05 * iIntensity * perturb);

    // Blend between the current frame and a slightly shifted down version of it using the max function
    let fragColor = max(c1, c2);

    // Fade out according to the beat
    let fragColor = fragColor * pow(defaultPulse, 0.3);

    // Fade out slowly
    let fadeAmount = 0.01 + 0.2 * (1. - iIntensity);
    let fragColor = max(fragColor - fadeAmount, vec4<f32>(0.));

    // Clear back buffer when intensity is low
    let fragColor = fragColor * smoothstep(0., 0.1, iIntensity);
    return fragColor;
}
