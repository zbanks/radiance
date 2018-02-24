#property description CGA mode 1: black, cyan, magenta, white

void main(void) {
    // This is pretty gross
    vec4 oc = texture(iInput, uv);
    vec3 c = oc.rgb;
    c = min(c / max(oc.a, 0.001), 1.);
    float bDist = -length(c - vec3(0., 0., 0.));
    float wDist = -length(c - vec3(1., 1., 1.));
    float cDist = -length(c - vec3(0., 1., 1.));
    float mDist = -length(c - vec3(1., 0., 1.));

    if (wDist > bDist && wDist > cDist && wDist > mDist) {
        c = vec3(1., 1., 1.);
    } else if (cDist > bDist && cDist > wDist && cDist > mDist) {
        c = vec3(0., 1., 1.);
    } else if (mDist > bDist && mDist > wDist && mDist > cDist) {
        c = vec3(1., 0., 1.);
    } else {
        c = vec3(0., 0., 0.);
    }

    fragColor.rgb = mix(oc.rgb, c, iIntensity * pow(defaultPulse, 2.));
    fragColor.a = oc.a;
}
