#property description Rainbow radial blur
#property frequency 1

let DEPTH = 16;

fn lookup(coord: vec2<f32>) -> vec4<f32> {
    return textureSample(iInputsTex[0], iSampler,  coord / aspectCorrection + 0.5);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let d = iIntensity * -normCoord / 200.;
    var w = iIntensity * 4. / f32(DEPTH);
    var s = normCoord;
    var col = lookup(s);
    let deltaHue = 1. / f32(DEPTH);
    for( var i=0; i<DEPTH; i++ )
    {
        w *= .9;
        s += d;
        let res = lookup(s);
        //let res = smoothstep(0., 1., res); // Makes colors more vibrant
        let res = res * max(res.r, max(res.g, res.b));
        let res = res * w;
        let hsv = rgb2hsv(res.rgb); // TODO this would probably be made much faster using a buffershader of the input converted to HSV
        let h = fract(hsv.x + f32(i) * deltaHue - 0.5 * iTime * iFrequency);
        let rgb = hsv2rgb(vec3<f32>(h, hsv.yz));
        col = composite(col, add_alpha(rgb, res.a));
    }

    // This composition results in some saturation loss,
    // so resaturate
    let hsv = rgb2hsv(col.rgb);
    let s = pow(hsv.y, (1.5 - pow(iIntensity, 0.5)) / 1.5);
    let rgb = hsv2rgb(vec3<f32>(hsv.x, s, hsv.z));

    return add_alpha(rgb, col.a);
}
