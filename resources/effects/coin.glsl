#property description Rotate the 'object' in 3D, like a coin

#define WIDTH 0.1
#define ITERS 64

vec4 lookup(vec2 coord) {
    vec2 xy = coord / aspectCorrection + 0.5;
    xy = clamp(xy, 0., 1.);
    return texture(iInput, xy);
}

void main() {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2  d = vec2(WIDTH, 0.) / float(ITERS);
    vec2  s = normCoord;
    //float phi = iIntensityIntegral * 4;
    float phi = iTime * 1.0;
    s.x /= sin(phi);

    // This isn't quite right, but it's super easy compared to real geometry
    d *= abs(cos(phi)) * sign(sin(phi * 2.));

    vec4 col = lookup(s);
    for( int i=0; i<ITERS; i++ )
    {
        s += d;
        vec4 res = lookup(s);
        col = composite(res, col);
    }

    fragColor = lookup(normCoord);
	fragColor = mix(fragColor, col, smoothstep(0.0, 0.2, iIntensity));
}
