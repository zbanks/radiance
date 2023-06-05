#property description Sort of like rainbow but for lightness, avoiding the edges
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let h = hsv.x;
    let s = hsv.y;
    let v = hsv.z;

    let spatialFrequency = 8.;
    let deviation = (iTime * iFrequency * 0.5) % 1.;
    let newLightness = v * ((spatialFrequency * v + deviation) % 1.);

    let amount = iIntensity;

    // Avoid the edges to smooth the discontinuity
    let amount = amount * smoothstep(0., 0.4, newLightness);
    let amount = amount * (1. - smoothstep(0.6, 1., newLightness));

    let v = mix(v, newLightness, amount);
    let rgb = hsv2rgb(vec3<f32>(h, s, v));
    return vec4<f32>(rgb, max(max(max(fragColor.a, rgb.r), rgb.g), rgb.b));
}
