#property description Rotate the 'object' in 3D, like a coin
#property frequency 1

let WIDTH = 0.1;
let ITERS = 64;

fn lookup(coord: vec2<f32>) -> vec4<f32> {
    let xy = coord / aspectCorrection + 0.5;
    return textureSample(iInputsTex[0], iSampler, xy) * box(xy);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    var d = vec2<f32>(WIDTH, 0.) / f32(ITERS);
    var s = normCoord;
    //float phi = iIntensityIntegral * 4;
    var phi = 0.;
    if (iFrequency != 0.) {
        phi = iTime * iFrequency * pi * 0.5;
    } else {
        phi = iIntensity * pi;
    }
    var s = vec2<f32>(s.x / cos(phi), s.y);

    // This isn't quite right, but it's super easy compared to real geometry
    d *= abs(cos(phi)) * sign(sin(phi * 2.));

    var col = lookup(s);
    for (var i = 0; i < ITERS; i++ )
    {
        s += d;
        let res = lookup(s);
        col = composite(res, col);
    }

    let c = mix(lookup(normCoord), col, smoothstep(0.0, 0.2, iIntensity));
    return c;
}

