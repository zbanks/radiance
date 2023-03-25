#property description CGA mode 1: black, cyan, magenta, white

fn main(uv: vec2<f32>) -> vec4<f32> {
    // This is pretty gross
    let oc = textureSample(iInputsTex[0], iSampler, uv);

    let bDist = -length(oc.rgb - vec3<f32>(0., 0., 0.));
    let wDist = -length(oc.rgb - vec3<f32>(1., 1., 1.));
    let cDist = -length(oc.rgb - vec3<f32>(0., 1., 1.));
    let mDist = -length(oc.rgb - vec3<f32>(1., 0., 1.));

    var c = vec4<f32>(0.);
    if wDist > bDist && wDist > cDist && wDist > mDist {
        c = vec4<f32>(1., 1., 1., 1.);
    } else if cDist > bDist && cDist > wDist && cDist > mDist {
        c = vec4<f32>(0., 1., 1., 1.);
    } else if mDist > bDist && mDist > wDist && mDist > cDist {
        c = vec4<f32>(1., 0., 1., 1.);
    } else {
        c = vec4<f32>(0., 0., 0., 1.);
    }

    return mix(oc, c, iIntensity * pow(defaultPulse, 2.));
}

