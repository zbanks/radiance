// Reduce number of colors

void main(void) {
    //float bins = 256. * pow(2, -8. * iIntensity);
    float bins = min(256., 1. / iIntensity);
    
    // bin in non-premultiplied space, then re-premultiply
    vec4 c = demultiply(texture2D(iInput, uv));
    gl_FragColor = premultiply(round(c * bins) / bins);
}
