#property description Projector light scattering

#define DEPTH 200

vec4 lookup(vec2 coord) {
    vec2 texUV = coord / aspectCorrection + 0.5;
    return texture(iInput, texUV) * box(texUV);
}

void main() {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec4 col = lookup(normCoord);
    vec2 source = vec2(0., -0.45);
    float w = 0.3 * iIntensity;
    for( int i=0; i<DEPTH; i++ )
    {
        w *= 0.98;
        vec2 s = (normCoord - source) / (i / float(DEPTH)) + source;
        vec4 res = lookup(s);
        res *= max(res.r, max(res.g, res.b));
        res *= w;
        col = composite(col, res);
    }

	fragColor = col;
}
