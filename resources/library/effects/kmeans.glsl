#property description k-means clustering? (idk)

void main(void) {
    vec2 uvCluster = texture(iChannel[1], uv).xy;
    vec2 uvNew = mix(uv, uvCluster, clamp(iIntensity * 2., 0., 1.0));
    fragColor = texture(iInput, uvNew);
    //fragColor = texture(iChannel[1], uv);
}

#buffershader

float distFn(vec4 x, vec4 y) {
    vec3 v = demultiply(x).xyz - demultiply(y).xyz;
    v = abs(v);
    //float d = v.x + v.y + v.z;
    float d = length(v);
    return d;
    //return 1. / (0.0001 + d);
}

void main(void) {
    //float q = pow(2.0, floor(rand(vec3(uv, iTime)) * 4));
    float q = 1.;
    vec2 a = uv;
    vec2 b = texture(iChannel[1], uv).xy;
    //vec2 a = texture(iChannel[1], uv + onePixel * vec2( 5,  5)).xy;
    //vec2 b = texture(iChannel[1], uv + onePixel * vec2(-5, -5)).xy;
    vec2 c = texture(iChannel[1], uv + onePixel * vec2( q,  q)).xy;
    vec2 d = texture(iChannel[1], uv + onePixel * vec2( q, -q)).xy;
    vec2 e = texture(iChannel[1], uv + onePixel * vec2(-q,  q)).xy;
    vec2 f = texture(iChannel[1], uv + onePixel * vec2(-q, -q)).xy;

    vec4 av = texture(iInput, a);
    vec4 bv = texture(iInput, b);
    vec4 cv = texture(iInput, c);
    vec4 dv = texture(iInput, d);
    vec4 ev = texture(iInput, e);
    vec4 fv = texture(iInput, f);

    vec4 mean = (av + bv + cv + dv + ev + fv) / 6.0;

    float ad = distFn(av, mean);
    float bd = distFn(bv, mean);
    float cd = distFn(cv, mean);
    float dd = distFn(dv, mean);
    float ed = distFn(ev, mean);
    float fd = distFn(fv, mean);

    float minDist = min(min(min(ad, bd), min(cd, dd)), min(ed, fd));

    float af = ad == minDist ? 1.0 : 0.0;
    float bf = bd == minDist ? 1.0 : 0.0;
    float cf = cd == minDist ? 1.0 : 0.0;
    float df = dd == minDist ? 1.0 : 0.0;
    float ef = ed == minDist ? 1.0 : 0.0;
    float ff = fd == minDist ? 1.0 : 0.0;
    float fsum = af + bf + cf + df + ef + ff;
    vec2 uvNew = (a * af + b * bf + c * cf + d * df + e * ef + f * ff) / (af + bf + cf + df + ef + ff);

    fragColor = vec4(uvNew, min(1.0, fsum * 0.5), 1.0);
    //fragColor = mean;
}
