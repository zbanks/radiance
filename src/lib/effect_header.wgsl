struct Uniforms {
    // Audio levels, high/mid/low/level, [0.0, 1.0]
    iAudio: vec4<f32>,

    // Time, measured in beats. Wraps around to 0 every 16 beats, [0.0, 16.0)
    iTime: f32,

    iFrequency: f32,

    // Intensity slider, [0.0, 1.0]
    iIntensity: f32,

    // Intensity slider integrated with respect to wall time mod 1024, [0.0, 1024.0)
    iIntensityIntegral: f32,

    // Resolution of the output pattern
    iResolution: vec2<f32>,

    // (Ideal) output rate in frames per second
    iStep: f32,
}

@group(0) @binding(0)
var<uniform> global: Uniforms;

@group(1) @binding(0)
var iSampler: sampler;

@group(1) @binding(1)
var iInputsTex: binding_array<texture_2d<f32>>;

@group(1) @binding(2)
var iNoiseTex: texture_2d<f32>;

@group(1) @binding(3)
var iChannelsTex: binding_array<texture_2d<f32>>;

var<private> iAudio: vec4<f32>;
var<private> iTime: f32;
var<private> iFrequency: f32;
var<private> iIntensity: f32;
var<private> iIntensityIntegral: f32;
var<private> iResolution: vec2<f32>;
var<private> iStep: f32;

var<private> aspectCorrection: vec2<f32>;
var<private> defaultPulse: f32;
var<private> onePixel: vec2<f32>;

// Aliases to audio levels
var<private> iAudioLow: f32;
var<private> iAudioMid: f32;
var<private> iAudioHi: f32;
var<private> iAudioLevel: f32;

const pi: f32 = 3.1415926535897932384626433832795;

fn modulo(x: f32, y: f32) -> f32 {
    return x - y * floor(x / y);
}

//float lin_step(float v) {
//    return v * iStep * iFPS;
//}
//float exp_step(float v) {
//    return pow(v, iStep * iFPS);
//}
//

// Utilities to convert from an RGB vec3 to an HSV vec3
// 0 <= H, S, V <= 1
fn rgb2hsv(c: vec3<f32>) -> vec3<f32> {
    let K = vec4<f32>(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    let p = mix(vec4<f32>(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    let q = mix(vec4<f32>(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    let d = q.x - min(q.w, q.y);
    let e = 1.0e-10;
    return vec3<f32>(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

fn hsv2rgb(c: vec3<f32>) -> vec3<f32> {
    let K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    let p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, vec3<f32>(0.0), vec3<f32>(1.0)), c.y);
}

// Utilities to convert from an RGB vec3 to a YUV vec3
// 0 <= Y, U, V <= 1 (*not* -1 <= U, V <= 1)
// U is greenish<->bluish; V is bluish<->redish
// https://en.wikipedia.org/wiki/YUV#Full_swing_for_BT.601
fn rgb2yuv(rgb: vec3<f32>) -> vec3<f32> {
    let y = rgb.r *  0.2126  + rgb.g *  0.7152  + rgb.b *  0.0722;
    let u = rgb.r * -0.09991 + rgb.g * -0.33609 + rgb.b *  0.436;
    let v = rgb.r *  0.615   + rgb.g * -0.55861 + rgb.b * -0.05639;
    return vec3(y, (u + 1.) * 0.5, (v + 1.) * 0.5);
}
fn yuv2rgb(yuv: vec3<f32>) -> vec3<f32> {
    let y = yuv.x;
    let u = yuv.y * 2. - 1.;
    let v = yuv.z * 2. - 1.;

    let r = y                + v *  1.28033;
    let g = y + u * -0.21482 + v * -0.38059;
    let b = y + u *  2.12798;
    let rgb = vec3<f32>(r, g, b);
    return clamp(rgb, vec3<f32>(0.0), vec3<f32>(1.0));
}


// Turn non-premultipled alpha RGBA into premultipled alpha RGBA
fn premultiply(c: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(c.rgb * c.a, c.a);
}

// Turn premultipled alpha RGBA into non-premultipled alpha RGBA
fn demultiply(c: vec4<f32>) -> vec4<f32> {
    return clamp(vec4<f32>(c.rgb / c.a, c.a), vec4<f32>(0.), vec4<f32>(1.));
}

// Alpha-compsite two colors, putting one on top of the other. Everything is premultipled
fn composite(under: vec4<f32>, over: vec4<f32>) -> vec4<f32> {
    let a_out: f32 = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4<f32>((over.rgb + under.rgb * (1. - over.a)), a_out), vec4<f32>(0.), vec4<f32>(1.));
}

// Sawtooth wave
fn sawtooth(x: f32, t_up: f32) -> f32 {
    let x2 = modulo(x + t_up, 1.);
    return x2 / t_up * step(x2, t_up) +
           (1. - x2) / (1. - t_up) * (1. - step(x2, t_up));
}

// Box from [0, 0] to (1, 1)
fn box(p: vec2<f32>) -> f32 {
    let b = step(vec2<f32>(0.), p) - step(vec2<f32>(1.), p);
    return b.x * b.y;
}


// Predictable randomness
fn rand(c: f32) -> f32 {
    return fract(sin(c * 12.9898) * 43758.5453);
}

fn rand2(c: vec2<f32>) -> f32 {
    return fract(sin(dot(c, vec2(12.9898,78.233))) * 43758.5453);
}

fn rand3(c: vec3<f32>) -> f32 {
    return fract(sin(dot(c, vec3(12.9898,78.233, 52.942))) * 43758.5453);
}

fn rand4(c: vec4<f32>) -> f32 {
    return fract(sin(dot(c, vec4(12.9898, 78.233, 52.942, 35.291))) * 43758.5453);
}

fn noise(p: f32) -> f32 {
    let i = i32(floor(p));
    let x1 = fract(p);
    // x = .5*(1.-cos(M_PI*x));
    let x2 = 3. * x1 * x1 - 2. * x1 * x1 * x1;
    let a = rand(f32(i + 0));
    let b = rand(f32(i + 1));
    return mix(a, b, x2);
}

fn noise2(p: vec2<f32>) -> f32 {
    let ij = vec2<i32>(floor(p));
    let xy1 = fract(p);
    // xy = .5*(1.-cos(M_PI*xy));
    let xy2 = 3. * xy1 * xy1 - 2. * xy1 * xy1 * xy1;
    let a = rand2(vec2<f32>(ij+vec2<i32>(0, 0)));
    let b = rand2(vec2<f32>(ij+vec2<i32>(1, 0)));
    let c = rand2(vec2<f32>(ij+vec2<i32>(0, 1)));
    let d = rand2(vec2<f32>(ij+vec2<i32>(1, 1)));
    let x1 = mix(a, b, xy2.x);
    let x2 = mix(c, d, xy2.x);
    return mix(x1, x2, xy2.y);
}

fn noise3(p: vec3<f32>) -> f32 {
    let ijk = vec3<i32>(floor(p));
    let xyz1 = fract(p);
    // xyz = .5*(1.-cos(M_PI*xyz));
    let xyz2 = 3. * xyz1 * xyz1 - 2. * xyz1 * xyz1 * xyz1;
    let a = rand3(vec3<f32>(ijk+vec3<i32>(0, 0, 0)));
    let b = rand3(vec3<f32>(ijk+vec3<i32>(1, 0, 0)));
    let c = rand3(vec3<f32>(ijk+vec3<i32>(0, 1, 0)));
    let d = rand3(vec3<f32>(ijk+vec3<i32>(1, 1, 0)));
    let e = rand3(vec3<f32>(ijk+vec3<i32>(0, 0, 1)));
    let f = rand3(vec3<f32>(ijk+vec3<i32>(1, 0, 1)));
    let g = rand3(vec3<f32>(ijk+vec3<i32>(0, 1, 1)));
    let h = rand3(vec3<f32>(ijk+vec3<i32>(1, 1, 1)));
    let x1 = mix(a, b, xyz2.x);
    let x2 = mix(c, d, xyz2.x);
    let y1 = mix(x1, x2, xyz2.y);
    let x3 = mix(e, f, xyz2.x);
    let x4 = mix(g, h, xyz2.x);
    let y2 = mix(x3, x4, xyz2.y);
    return mix(y1, y2, xyz2.z);
}

fn noise4(p: vec4<f32>) -> f32 {
    let ijkl = vec4<i32>(floor(p));
    let xyzw1 = fract(p);
    // xyz = .5*(1.-cos(M_PI*xyz));
    let xyzw2 = 3. * xyzw1 * xyzw1 - 1. * xyzw1 * xyzw1 * xyzw1;
    let a = rand4(vec4<f32>(ijkl+vec4<i32>(0, 0, 0, 0)));
    let b = rand4(vec4<f32>(ijkl+vec4<i32>(1, 0, 0, 0)));
    let c = rand4(vec4<f32>(ijkl+vec4<i32>(0, 1, 0, 0)));
    let d = rand4(vec4<f32>(ijkl+vec4<i32>(1, 1, 0, 0)));
    let e = rand4(vec4<f32>(ijkl+vec4<i32>(0, 0, 1, 0)));
    let f = rand4(vec4<f32>(ijkl+vec4<i32>(1, 0, 1, 0)));
    let g = rand4(vec4<f32>(ijkl+vec4<i32>(0, 1, 1, 0)));
    let h = rand4(vec4<f32>(ijkl+vec4<i32>(1, 1, 1, 0)));
    let i = rand4(vec4<f32>(ijkl+vec4<i32>(0, 0, 0, 1)));
    let j = rand4(vec4<f32>(ijkl+vec4<i32>(1, 0, 0, 1)));
    let k = rand4(vec4<f32>(ijkl+vec4<i32>(0, 1, 0, 1)));
    let l = rand4(vec4<f32>(ijkl+vec4<i32>(1, 1, 0, 1)));
    let m = rand4(vec4<f32>(ijkl+vec4<i32>(0, 0, 1, 1)));
    let n = rand4(vec4<f32>(ijkl+vec4<i32>(1, 0, 1, 1)));
    let o = rand4(vec4<f32>(ijkl+vec4<i32>(0, 1, 1, 1)));
    let q = rand4(vec4<f32>(ijkl+vec4<i32>(1, 1, 1, 1)));
    let x1 = mix(a, b, xyzw2.x);
    let x2 = mix(c, d, xyzw2.x);
    let y1 = mix(x1, x2, xyzw2.y);
    let x3 = mix(e, f, xyzw2.x);
    let x4 = mix(g, h, xyzw2.x);
    let y2 = mix(x3, x4, xyzw2.y);
    let z1 = mix(y1, y2, xyzw2.z);

    let x5 = mix(i, j, xyzw2.x);
    let x6 = mix(k, l, xyzw2.x);
    let y3 = mix(x5, x6, xyzw2.y);
    let x7 = mix(m, n, xyzw2.x);
    let x8 = mix(o, q, xyzw2.x);
    let y4 = mix(x7, x8, xyzw2.y);
    let z2 = mix(y3, y4, xyzw2.z);
    return mix(z1, z2, xyzw2.w);
}

//float hmax(vec2 v) {
//    return max(v.r,v.g);
//}
//float hmax(vec3 v) {
//    return max(hmax(v.rg),v.b);
//}
//float hmax(vec4 v) {
//    return hmax(max(v.rg,v.ba));
//}

fn inverse2(m: mat2x2<f32>) -> mat2x2<f32> {
    let a = m[0][0];
    let b = m[1][0];
    let c = m[0][1];
    let d = m[1][1];

    let det = a * d - b * c;

    return mat2x2<f32>(d, -c, -b, a) * (1. / det);
}

// Source adapted from https://github.com/glslify/glsl-inverse/blob/master/index.glsl
fn inverse3(m: mat3x3<f32>) -> mat3x3<f32> {
    let a00 = m[0][0];
    let a01 = m[0][1];
    let a02 = m[0][2];
    let a10 = m[1][0];
    let a11 = m[1][1];
    let a12 = m[1][2];
    let a20 = m[2][0];
    let a21 = m[2][1];
    let a22 = m[2][2];

    let b01 = a22 * a11 - a12 * a21;
    let b11 = -a22 * a10 + a12 * a20;
    let b21 = a21 * a10 - a11 * a20;

    let det = a00 * b01 + a01 * b11 + a02 * b21;

    return mat3x3<f32>(
        b01, (-a22 * a01 + a02 * a21), (a12 * a01 - a02 * a11),
        b11, (a22 * a00 - a02 * a20), (-a12 * a00 + a02 * a10),
        b21, (-a21 * a00 + a01 * a20), (a11 * a00 - a01 * a10),
    ) * (1. / det);
}

// From https://gist.github.com/rsms/9d9e7c4eadf9fe23da0bf0bfb96bc2e6#file-webgpu-infinite-world-grid-cc-L26
fn inverse4(m: mat4x4<f32>) -> mat4x4<f32> {
    // Note: wgsl does not have an inverse() (matrix inverse) function built in.
    // Source adapted from https://github.com/glslify/glsl-inverse/blob/master/index.glsl
    let a00 = m[0][0];
    let a01 = m[0][1];
    let a02 = m[0][2];
    let a03 = m[0][3];
    let a10 = m[1][0];
    let a11 = m[1][1];
    let a12 = m[1][2];
    let a13 = m[1][3];
    let a20 = m[2][0];
    let a21 = m[2][1];
    let a22 = m[2][2];
    let a23 = m[2][3];
    let a30 = m[3][0];
    let a31 = m[3][1];
    let a32 = m[3][2];
    let a33 = m[3][3];
    let b00 = a00 * a11 - a01 * a10;
    let b01 = a00 * a12 - a02 * a10;
    let b02 = a00 * a13 - a03 * a10;
    let b03 = a01 * a12 - a02 * a11;
    let b04 = a01 * a13 - a03 * a11;
    let b05 = a02 * a13 - a03 * a12;
    let b06 = a20 * a31 - a21 * a30;
    let b07 = a20 * a32 - a22 * a30;
    let b08 = a20 * a33 - a23 * a30;
    let b09 = a21 * a32 - a22 * a31;
    let b10 = a21 * a33 - a23 * a31;
    let b11 = a22 * a33 - a23 * a32;
    let det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
    return mat4x4<f32>(
      vec4<f32>(
        a11 * b11 - a12 * b10 + a13 * b09,
        a02 * b10 - a01 * b11 - a03 * b09,
        a31 * b05 - a32 * b04 + a33 * b03,
        a22 * b04 - a21 * b05 - a23 * b03),
      vec4<f32>(
        a12 * b08 - a10 * b11 - a13 * b07,
        a00 * b11 - a02 * b08 + a03 * b07,
        a32 * b02 - a30 * b05 - a33 * b01,
        a20 * b05 - a22 * b02 + a23 * b01),
      vec4<f32>(
        a10 * b10 - a11 * b08 + a13 * b06,
        a01 * b08 - a00 * b10 - a03 * b06,
        a30 * b04 - a31 * b02 + a33 * b00,
        a21 * b02 - a20 * b04 - a23 * b00),
      vec4<f32>(
        a11 * b07 - a10 * b09 - a12 * b06,
        a00 * b09 - a01 * b07 + a02 * b06,
        a31 * b01 - a30 * b03 - a32 * b00,
        a20 * b03 - a21 * b01 + a22 * b00)
    ) * (1.0 / det);
}

// Adds back in the alpha channel for a modified RGB color
// dimming the color as necessary
fn add_alpha_dimming(rgb: vec3<f32>, a: f32) -> vec4<f32> {
    let min_new_alpha = max(max(rgb.r, rgb.g), rgb.b);
    let scale = min(1., a / min_new_alpha);
    return vec4<f32>(rgb * scale, a);
}

// Adds back in the alpha channel for a modified RGB color
// increasing opacity as necessary
fn add_alpha(rgb: vec3<f32>, a: f32) -> vec4<f32> {
    return vec4<f32>(rgb, max(max(max(rgb.r, rgb.g), rgb.b), a));
}

//#line 1
