#property description Apply `uvmapself` repeatedly. Instant hellscape

let N = 5;

fn main(uv: vec2<f32>) -> vec4<f32> {
    // It looks better at low values of intensity
    let intensity = pow(iIntensity, 3.0);

    var newUV = uv;
    for (var i = 0; i < N; i++) {
        let map = textureSample(iInputsTex[0], iSampler,  newUV);
        newUV = mix(newUV, map.rg, intensity * map.a * pow(defaultPulse, 2.));
    }
    return textureSample(iInputsTex[0], iSampler,  newUV);
}
