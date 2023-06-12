#property description Rotating squares
#property inputCount 2
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let x = iIntensity;
    // Dead width 
    let dw = 0.22;

    // TODO: what's the right value for '0.3'?
    //float scale = pow(32.0, iIntensity) * 0.3;
    let scale = 4.0;
    let t = iTime * iFrequency * 0.25;

    let td = fract(t * 4.0);
    let tw = floor(t * 4.0);
    let td = tw + smoothstep(0., 1., td);
    let td = td / (2.0);
    // t with detent
    //let t = mix(t, td, 0.97);

    let q = step(fract(t + 0.25), 0.5);
    let q = min(q, step(0.5 - dw * 0.5, x));
    let q = max(q, step(0.5 + dw * 0.5, x));

    let theta = t * pi;
    let uvNorm = (uv - 0.5) * scale * aspectCorrection;
    let r1 = fract(uvNorm);
    let r2 = fract(uvNorm + vec2<f32>(0.5));
    let rep = mix(r1, r2, q);
    let off = (rep - vec2<f32>(0.5, 0.5)) * 2.0;

    let s = sin(theta);
    let c = cos(theta);
    let rot = mat2x2<f32>(c, -s, s, c);
    let xy = off * rot;
    let eps = 0.05;
    let slope = (1.0 + dw / 2.0);
    let d = 1.0 / sqrt(2.0) * min(1.0, 2.0 * ((1.0 + dw / 2.0) * (0.5 - abs(x - 0.5))));
    let v = smoothstep(d - eps, d + eps, max(abs(xy.x), abs(xy.y)));

    let left = vec4<f32>(1.0, 1.0, 1.0, 1.0);
    let right = vec4<f32>(0.0, 0.0, 0.0, 1.0);
    
    let left = textureSample(iInputsTex[0], iSampler,  uv);
    let right = textureSample(iInputsTex[1], iSampler,  uv);

    return mix(right, left, abs(q - v));
}
