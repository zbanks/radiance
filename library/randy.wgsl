#property description Obnoxiously zoom and rotate ( in honor of  Raaaaandy Seidman )
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    let t = iTime * iFrequency;

    let theta = 0.3 * (cos(t * 0.6) - sin(2. * t * 0.6)) * iIntensity;
    let zoom = 1. + 0.2 * (-1. + sin(t * 0.35) - cos(2. * t * 0.35)) * iIntensity;

    let s = sin(theta) / zoom;
    let c = cos(theta) / zoom;
    let rot = mat2x2<f32>(c, -s, s, c);

    let newUV = normCoord * rot;
    let newUV = newUV / aspectCorrection + 0.5;

    let nc = textureSample(iInputsTex[0], iSampler,  newUV);
    let nc = nc * (box(newUV));

    return nc;
}
