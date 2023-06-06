#property description Like you're looking at the image reflected in something that was surfaced on a lathe
#property frequency 0.25

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5);
    let normCoord = normCoord * (aspectCorrection);

    // Look up r and theta
    let r = length(normCoord);
    let theta = atan2(normCoord.y, normCoord.x);

    // Image is sampled along a vertical line that slowly shifts back and forth
    // and this line is then swept into a circle
    // Get two newUVs, one for the top half and one for the bottom half
    let newUV1 = vec2<f32>(sin(iTime * iFrequency * 0.5 * pi), r);
    let newUV2 = vec2<f32>(sin(iTime * iFrequency * 0.5 * pi), -r);
    let newUV1 = newUV1 / (aspectCorrection);
    let newUV1 = newUV1 * 0.5 + 0.5;
    let newUV2 = newUV2 / (aspectCorrection);
    let newUV2 = newUV2 * 0.5 + 0.5;

    // Make them both converge to the old UV at low intensity so that identity holds
    let newUV1 = mix(uv, newUV1, smoothstep(0., 0.3, iIntensity));
    let newUV2 = mix(uv, newUV2, smoothstep(0., 0.3, iIntensity));

    let c1 = textureSample(iInputsTex[0], iSampler,  newUV1);
    let c2 = textureSample(iInputsTex[0], iSampler,  newUV2);

    // Mix them based on angle
    return mix(c2, c1, smoothstep(-0.5, 0.5, sin(theta)));
}
