#property description Overlays a smaller pattern and zoom in on it
#property frequency 0.5

let N_COPIES = 8;

fn lookup(uv: vec2<f32>, scale: f32) -> vec4<f32>{
    let newUV = (uv - 0.5) * scale + 0.5;
    return textureSample(iInputsTex[0], iSampler,  newUV) * box(newUV);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let scaleReduction = 3. * iIntensity + 1.;
    let t = -((iTime * iFrequency * 0.5) % 1.);
    let alpha = -t;
    var scale = pow(scaleReduction, t);
    // fade out closest copy
    var fragColor = lookup(uv, scale) * (1. - alpha);
    scale = scale * scaleReduction;
    for (var i=0; i < N_COPIES - 2; i++) {
        fragColor = composite(fragColor, lookup(uv, scale));
        scale = scale * scaleReduction;
    }
    // fade in farthest copy
    fragColor = composite(fragColor, lookup(uv, scale) * alpha);
    return fragColor;
}
