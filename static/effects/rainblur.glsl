#property description Rainbow radial blur

#define DEPTH 16

vec4 lookup(vec2 coord) {
    return texture(iInput, coord / aspectCorrection + 0.5);
}

void main() {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2  d = iIntensity * -normCoord / 200.;
    float w = iIntensity * 4. / float(DEPTH);
    vec2  s = normCoord;
    vec4 col = lookup(s);
    float deltaHue = 1. / float(DEPTH);
    for( int i=0; i<DEPTH; i++ )
    {
        w *= .9;
        s += d;
        vec4 res = lookup(s);
        //res = smoothstep(0., 1., res); // Makes colors more vibrant
        res *= max(res.r, max(res.g, res.b));
        res *= w;
        res.xyz = rgb2hsv(res.rgb); // TODO this would probably be made much faster using a buffershader of the input converted to HSV
        res.x = mod(res.x + float(i) * deltaHue - 0.5 * iIntensityIntegral, 1.);
        res.rgb = hsv2rgb(res.xyz);
        col = composite(col, res);
    }

    // This composition results in some saturation loss,
    // so resaturate
    vec3 hsv = rgb2hsv(col.rgb);
    hsv.y = pow(hsv.y, (2. - iIntensity) / 2.);
    col.rgb = hsv2rgb(hsv);

	fragColor = col;
}
