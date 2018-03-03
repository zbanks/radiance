#property description Matlab-ify the colors
#property author https://github.com/kbinani/glsl-colormap

// [ From https://github.com/kbinani/glsl-colormap/blob/master/shaders/MATLAB_jet.frag
float colormap_red(float x) {
    if (x < 0.7) {
        return 4.0 * x - 1.5;
    } else {
        return -4.0 * x + 4.5;
    }
}
float colormap_green(float x) {
    if (x < 0.5) {
        return 4.0 * x - 0.5;
    } else {
        return -4.0 * x + 3.5;
    }
}
float colormap_blue(float x) {
    if (x < 0.3) {
       return 4.0 * x + 0.5;
    } else {
       return -4.0 * x + 2.5;
    }
}
vec4 colormap(float x) {
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
// ]

void main(void) {
    vec4 origColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(demultiply(origColor).rgb);

    float h = mod(0.90 - hsv.x, 1.0);
    vec4 c = colormap(h);                   // Hue
    c.rgb = mix(vec3(1.0), c.rgb, hsv.y);   // Sat
    c.rgb *= hsv.z;                         // Val
    c *= origColor.a;                       // Alpha

    fragColor = mix(origColor, c, iIntensity * pow(defaultPulse, 2.));
}
