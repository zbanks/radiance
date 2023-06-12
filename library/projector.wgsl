#property description Projector light scattering

let DEPTH = 200;

fn lookup(coord: vec2<f32>) -> vec4<f32> {
    let texUV = coord / aspectCorrection + 0.5;
    return textureSample(iInputsTex[0], iSampler,  texUV) * box(texUV);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    var col = lookup(normCoord);
    let angle = iTime * iFrequency * 0.25;
    let source = 0.45 * vec2<f32>(sin(angle), cos(angle));
    var w = 0.3 * iIntensity;
    for (var i=0; i<DEPTH; i++) {
        w *= 0.98;
        let s = (normCoord - source) / (f32(i) / f32(DEPTH)) + source;
        let res = lookup(s);
        let res = res * (max(res.r, max(res.g, res.b)));
        let res = res * (w);
        col = composite(col, res);
    }

    return col;
}
