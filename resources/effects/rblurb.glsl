// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for Radiance by Eric Van Albert

#define DEPTH 64

vec4 lookup(vec2 coord) {
    return texture(iInput, coord / aspectCorrection + 0.5);
}

void main() {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2  d = -normCoord / DEPTH.;
    float w = iIntensity * iAudioLow * 7. / DEPTH.;
    vec2  s = normCoord;
    vec4 col = lookup(s);
    for( int i=0; i<DEPTH; i++ )
    {
        w *= .99;
        s += d;
        vec4 res = lookup(s);
        //res = smoothstep(0., 1., res); // Makes colors more vibrant
        res *= max(res.r, max(res.g, res.b));
        res *= w;
        col = composite(col, res);
    }

	fragColor = col;
}
