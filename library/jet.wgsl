#property description Matlab-ify the colors
//#property author https://github.com/kbinani/glsl-colormap

// [ From https://github.com/kbinani/glsl-colormap/blob/master/shaders/MATLAB_jet.frag
fn colormap_red(x: f32) -> f32{
    if (x < 0.7) {
        return 4.0 * x - 1.5;
    } else {
        return -4.0 * x + 4.5;
    }
}
fn colormap_green(x: f32) -> f32 {
    if (x < 0.5) {
        return 4.0 * x - 0.5;
    } else {
        return -4.0 * x + 3.5;
    }
}
fn colormap_blue(x: f32) -> f32 {
    if (x < 0.3) {
       return 4.0 * x + 0.5;
    } else {
       return -4.0 * x + 2.5;
    }
}
fn colormap(x: f32) -> vec3<f32> {
    let r = clamp(colormap_red(x), 0.0, 1.0);
    let g = clamp(colormap_green(x), 0.0, 1.0);
    let b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec3<f32>(r, g, b);
}
// ]

fn main(uv: vec2<f32>) -> vec4<f32> {
    let origColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(demultiply(origColor).rgb);

    let h = (1.90 - hsv.x) % 1.0;
    let c = colormap(h);                   // Hue
    let c = mix(vec3<f32>(1.0), c.rgb, hsv.y);   // Sat
    let c = c * hsv.z;                         // Val
    let c = vec4<f32>(c, 1.) * origColor.a;

    return mix(origColor, c, iIntensity * pow(defaultPulse, 2.));
}
