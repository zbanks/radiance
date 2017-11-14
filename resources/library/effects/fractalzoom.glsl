#property description Overlays a smaller pattern and zoom in on it

#define N_COPIES 5

vec4 lookup(float scale) {
    vec2 newUV = (uv - 0.5) * scale + 0.5;
    return texture(iInput, newUV) * box(newUV);
}

void main(void) {
    float scaleReduction = 15. * iIntensity + 1.;
    float t = mod(-iIntensityIntegral * 0.5, 1.);
    float scale = pow(scaleReduction, t - 2.);
    fragColor = lookup(scale) * t;
    scale *= scaleReduction;
    for (int i=0; i<N_COPIES - 2; i++) {
        fragColor = composite(fragColor, lookup(scale));
        scale *= scaleReduction;
    }
    fragColor = composite(fragColor, lookup(scale) * (1. - t));
    scale *= scaleReduction;
}
