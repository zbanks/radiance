#property description Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for Radiance by Eric Van Albert

let MAX_DEPTH = 12;

fn lookup(coord: vec2<f32>) -> vec4<f32> {
    let uv = coord / aspectCorrection + 0.5;
    return textureSample(iInputsTex[0], iSampler,  uv) * box(uv);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let depth = iIntensity * f32(MAX_DEPTH) * (0.5 + 0.5 * pow(defaultPulse, 2.));
    let normCoord = (uv - 0.5) * aspectCorrection;
    let d = -normCoord / depth;
    var w = 3. / depth;
    var s = normCoord;
    var col = lookup(s);
    for (var i=0; i<MAX_DEPTH; i++) {
        if (f32(i) >= depth) {
            break;
        }

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
