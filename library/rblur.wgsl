#property description Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for Radiance by Eric Van Albert

let DEPTH = 64;

fn lookup(coord: vec2<f32>) -> vec4<f32> {
    return textureSample(iInputsTex[0], iSampler,  coord / aspectCorrection + 0.5);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let d = -normCoord / f32(DEPTH);
    var w = iIntensity * 4. / f32(DEPTH) * pow(defaultPulse, 1.);
    var s = normCoord;
    var col = lookup(s);
    for( var i=0; i<DEPTH; i++ )
    {
        w *= .99;
        s += d;
        let res = lookup(s);
        //let res = smoothstep(0., 1., res); // Makes colors more vibrant
        let res = res * (max(res.r, max(res.g, res.b)));
        let res = res * (w);
        col = composite(col, res);
    }

    return col;
}
