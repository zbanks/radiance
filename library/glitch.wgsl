#property description Digital glitching

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler,  uv);

    var N_PARTITIONS: i32 = 3;
    var blockSizes = array<vec2<f32>, 3>(
        vec2<f32>(0.3, 0.5),
        vec2<f32>(1., 0.15),
        vec2<f32>(1., 1.),
    );

    // Partition image into blocks a few different ways
    var n1 = vec4<f32>(1.);
    var n2 = vec4<f32>(1.);
    for (var i=0; i<N_PARTITIONS; i++) {
        let block = floor(uv / blockSizes[i]) * blockSizes[i];
        let t1 = textureSample(iNoiseTex, iSampler,  vec2<f32>(iTime / 1000., (3.1 * block.x) + (7.8 * block.y)));
        n1 *= t1;
        let t2 = textureSample(iNoiseTex, iSampler,  vec2<f32>(iTime / 1000., (6.7 * block.x) + (2.9 * block.y)));
        n2 *= t2;
    }
    n1 = pow(n1, vec4<f32>(0.1));
    n2 = pow(n2, vec4<f32>(0.1));

    let parameter = iIntensity * (0.5 + 0.5 * pow(defaultPulse, 2.));

    // White noise glitch
    let noise_glitch = step(1. - 0.2 * parameter, n1.x);
    let white_noise = textureSample(iNoiseTex, iSampler, vec2<f32>(rand3(vec3<f32>(uv, iTime + 100.)), rand3(vec3<f32>(uv, iTime))) % 1.);
    let white_noise_rgb = white_noise.rgb * (white_noise.a);
    let c = mix(c, vec4<f32>(white_noise_rgb, white_noise.a), noise_glitch);

    // Invert colors
    let invert_glitch = step(1. - 0.2 * parameter, n1.y);
    let c = mix(c, vec4<f32>(c.a - c.rgb, c.a), invert_glitch);

    // Solid color glitch
    let solid_glitch = step(1. - 0.2 * parameter, n1.z);
    let c = mix(c, vec4<f32>(0., 1., 0., 1.), solid_glitch);

    // Shift glitch
    let shift_glitch = step(1. - 0.2 * parameter, n1.w);
    let c = mix(c, textureSample(iInputsTex[0], iSampler,  uv - vec2<f32>(0.2, 0.)), shift_glitch);

    // Freeze glitch
    let freeze_glitch = step(1. - 0.2 * parameter, n2.x);
    let c = mix(c, textureSample(iChannelsTex[0], iSampler,  uv), freeze_glitch);

    return c;
}
