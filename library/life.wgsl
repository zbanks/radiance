#property description Game of life?

fn main(uv: vec2<f32>) -> vec4<f32> {
    let alive = textureSample(iChannelsTex[1], iSampler,  uv).r;
    let under = textureSample(iInputsTex[0], iSampler,  uv);
    let over = alive * textureSample(iChannelsTex[2], iSampler,  uv);
    let over = over * (smoothstep(0., 0.2, iIntensity));
    return composite(under, over);
}

#buffershader

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    //float bs = 2048. * pow(2, -5. * iIntensity);
    let bs = max((1. - iIntensity) / (2.3 * onePixel.x), 5.);
    let bins = bs * aspectCorrection;
    let db = 1. / (bins * aspectCorrection);
    let normCoord = round(normCoord * bins) * db + 0.5;

    let n = 0.;
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>(-1., -1.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>(-1.,  0.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>(-1.,  1.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>( 0., -1.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>( 0.,  1.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>( 1., -1.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>( 1.,  0.)).r);
    let n = n + (textureSample(iChannelsTex[1], iSampler,  normCoord + db * vec2<f32>( 1.,  1.)).r);
    let s = textureSample(iChannelsTex[1], iSampler,  normCoord).r;


    // Use bright areas of the source image to help "birth" pixels (or kill)
    let source = textureSample(iInputsTex[0], iSampler,  normCoord) * pow(defaultPulse, 2.);
    //float r = 20. * rand(vec3<f32>(normCoord, iTime)) + mix(4.0, 0, iIntensity);
    //float bonus = step(20.5, r + max(max(source.r, source.g), source.b));
    //let n = //n + (bonus * 3);

    // if (s == 0) { alive = (n == 3) }
    // else { alive = (2 <= n <= 3) }
    let alive = step(1.8, n) * step(n, 3.2) * step(2.8, n + s);
    let alive = alive * step(0.05, iIntensity); // to reset

    // Make there be life if there is sufficient input color
    //float lifeFromInput = step(0.5, smoothstep(0., 3., dot(vec3<f32>(1.), source.rgb)));
    let lifeFromInput = step(0.8, max(source.r, max(source.g, source.b)));
    let alive = max(alive, lifeFromInput);
    let alive = alive * step(0.01, textureSample(iChannelsTex[2], iSampler,  normCoord).a); // Kill stable life if there is no color

    return vec4<f32>(alive, 1., 1., 1.);
}

#buffershader

// This buffer just paints the world so that life can extend
// outside of what currently has color

fn main(uv: vec2<f32>) -> vec4<f32> {
    let oldC = textureSample(iChannelsTex[2], iSampler,  uv);
    let d = mix(0.01, 0.001, iIntensity);
    let oldC = max(oldC - d, vec4<f32>(0.));
    let newC = textureSample(iInputsTex[0], iSampler,  uv);
    return max(oldC, newC);
}
