// Rotate the 'object' in 3D, like a coin

#define WIDTH 0.1
#define ITERS 64

vec4 lookup(vec2 coord) {
    return texture2D(iInput, coord / aspectCorrection + 0.5);
}

void main() {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    vec2  d = vec2(WIDTH, 0.) / ITERS.;
    vec2  s = normCoord;
    float phi = iIntensityIntegral * 4;
    s.x /= sin(phi);

    // This isn't quite right, but it's super easy compared to real geometry
    d *= abs(cos(phi)) * sign(sin(phi * 2));

    vec4 col = lookup(s);
    for( int i=0; i<ITERS; i++ )
    {
        s += d;
        vec4 res = lookup(s);
        col = composite(res, col);
    }

    gl_FragColor = lookup(normCoord);
	gl_FragColor = mix(gl_FragColor, col, smoothstep(0.0, 0.2, iIntensity));
}
