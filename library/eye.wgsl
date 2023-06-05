#property description Distort effect that makes the image look more like an eye
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5) * aspectCorrection;

    //let lengthFactor = sqrt(2.);
    let lengthFactor = 1.0;

    let len = length(normCoord) / lengthFactor;

    let angle_fwd = abs((10. + 2. * atan2(normCoord.x, normCoord.y) / pi + 0.25 * iTime * iFrequency) % 4. - 2.) - 1.;
    let angle_rev = abs((10. + 2. * atan2(normCoord.y, normCoord.x) / pi + 0.25 * iTime * iFrequency) % 4. - 2.) - 1.;

    let newUVCenter = vec2<f32>(angle_fwd, len) - 0.5;
    let newUVMiddle = vec2<f32>(len - 1., angle_rev) - 0.5;
    let newUVOutside = 0.5 * normCoord;

    //let a = iIntensity * 0.2 * (0.5 + 0.5 * pow(defaultPulse, 2.)); // Make the eye open a little
    let a = iIntensity * 0.5;
    let centerShape = smoothstep(0.1 + a, 0.3 + a, len);
    let eyeShape = smoothstep(0.8 + a, 1.0 + a, length(abs(normCoord) + vec2<f32>(0., 0.7)));

    let newUV = mix(newUVCenter, newUVMiddle, centerShape);
    let newUV = mix(newUV, newUVOutside, eyeShape);

    let newUV = newUV / aspectCorrection + 0.5;

    return textureSample(iInputsTex[0], iSampler,  mix(uv, newUV, iIntensity));
}
