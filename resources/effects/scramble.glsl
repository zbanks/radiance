#property description Scramble up input blocks

void main(void) {
    vec4 secondary = texture(iChannel[1], uv);
    vec2 uvNew = mix(uv, secondary.xy, smoothstep(0.0, 0.2, iIntensity));
    fragColor = texture(iInput, uvNew);
}

#buffershader

vec2 pixelate(vec2 xy, float n_buckets) {
    vec2 xy_buckets = n_buckets * aspectCorrection;
    xy -= 0.5;
    xy = round(xy * xy_buckets) / xy_buckets;
    xy += 0.5;
    return xy;
}

bool in_bucket(vec2 uv, vec2 xy, float n_buckets) {
    vec2 d = abs(uv - xy) * n_buckets;
    return max(d.x, d.y) <= 0.5;
}

void main(void) {
    float n_buckets = 10;

    vec2 left = vec2(rand(vec2(iTime, 0.)), rand(vec2(iTime, 1.)));
    left = pixelate(left, n_buckets);
    vec2 right = vec2(rand(vec2(iTime, 2.)), rand(vec2(iTime, 3.)));
    right = pixelate(right, n_buckets);

    vec4 newColor = texture(iChannel[1], uv);
    if (in_bucket(uv, left, n_buckets)) {
        vec2 newCoord = uv - left + right;
        float oldDist = distance(newColor.xy, uv);
        float newDist = distance(newCoord, uv);
        float improvement = oldDist - newDist; // positive means reverting to normal
        if (improvement > -1.8 * iIntensity + 0.5) {
            newColor = vec4(newCoord, 1., 1.);
        }
        if (iIntensity < 0.3) {
            newColor = vec4(uv.xy, 1., 1.);
        }
    }

    fragColor = mix(
        vec4(uv.xy, 1., 1.),
        newColor,
        step(0.05, iIntensity)
    );
}
