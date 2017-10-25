#property description Apply a Dunkirk-esque (dark blue) palette

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(fragColor.rgb);
    float h = hsv.x;
    h = mod(h - 1. / 12., 1.0) - 6. / 12.;
    h *= (1. - iIntensity * 0.7);
    h = mod(h + 7. / 12., 1.0);
    hsv.x = h;
    hsv.y = mix(hsv.y, 0., iIntensity * 0.4);
    hsv.z = mix(hsv.z, 0., iIntensity * 0.3);
    fragColor.rgb = hsv2rgb(hsv);
}
