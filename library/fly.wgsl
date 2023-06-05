#property description 3D flying view
//#property author inigo quilez (iq/2013), modified by Eric Van Albert
#property frequency 1
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for radiance by Eric Van Albert

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5) * aspectCorrection;

    let an = iTime * iFrequency * pi / 16.;
    let nc_spin = mat2x2<f32>(cos(an),-sin(an),sin(an),cos(an)) * normCoord;
    let nc_fly = vec2<f32>(nc_spin.x,1.)/abs(nc_spin.y) + vec2<f32>(0., 2.) * iIntensityIntegral;
    let nc_tile = abs((0.2 * nc_fly + 21.) % 4. - 2.) - 1.;

    let nc_tile = mix(normCoord, nc_tile, smoothstep(0.0, 0.2, iIntensity));
    let c = textureSample(iInputsTex[0], iSampler,  nc_tile / (2. * aspectCorrection) + 0.5);
    let c = c * mix(1., abs(nc_spin.y), smoothstep(0.0, 0.1, iIntensity));
    return c;
}
