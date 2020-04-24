#property description Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for Radiance by Eric Van Albert

#define MAX_DEPTH 12

vec4 lookup(vec2 coord) {
    return texture(iInput, coord / aspectCorrection + 0.5);
}

void main() {
    float depth = iIntensity * float(MAX_DEPTH);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2  d = -normCoord / depth;
    float w = 3. / depth;
    vec2  s = normCoord;
    vec4 col = lookup(s);
    for( int i=0; i<MAX_DEPTH; i++ ) {
        if (float(i) >= depth)
            break;

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
