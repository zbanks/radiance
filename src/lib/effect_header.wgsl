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

@group(0) @binding(1)
var iSampler: sampler;

@group(0) @binding(2)
var iInputsTex: binding_array<texture_2d<f32>>;

@group(0) @binding(3)
var iNoiseTex: texture_2d<f32>;

@group(0) @binding(4)
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

//// Outputs of previous patterns
//layout(set = 1, binding = 1) uniform texture2D iInputsTex[];
//
//// Full frame RGBA noise
//layout(set = 1, binding = 2) uniform texture2D iNoiseTex;
//
//// Previous outputs of the other channels (e.g. foo.1.glsl)
////layout(set = 1, binding = 3) uniform texture2D iChannelTexXXX[];
//
//// Macros to approximate the OpenGL syntax
//#define iInputs(X) (sampler2D(iInputsTex[(X)], iSampler))
//#define iNoise (sampler2D(iNoiseTex, iSampler))
//#define iChannel(X) (sampler2D(iChannelTex[(X)], iSampler))
//
//// Output of the previous pattern.  Alias to iInputs(0)
//#define iInput iInputs(0)
//
//// Aliases to audio levels
//#define iAudioLow   iAudio.x
//#define iAudioMid   iAudio.y
//#define iAudioHi    iAudio.z
//#define iAudioLevel iAudio.w
//

let pi: f32 = 3.1415926535897932384626433832795;

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
//// Utilities to convert from an RGB vec3 to an HSV vec3
//// 0 <= H, S, V <= 1
//vec3 rgb2hsv(vec3 c) {
//    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
//    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
//    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
//
//    float d = q.x - min(q.w, q.y);
//    float e = 1.0e-10;
//    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
//}
//
//vec3 hsv2rgb(vec3 c) {
//    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
//    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
//    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
//}
//
//// Utilities to convert from an RGB vec3 to a YUV vec3
//// 0 <= Y, U, V <= 1 (*not* -1 <= U, V <= 1)
//// U is greenish<->bluish; V is bluish<->redish
//// https://en.wikipedia.org/wiki/YUV#Full_swing_for_BT.601
//vec3 rgb2yuv(vec3 rgb) {
//    vec3 yuv = vec3(0.);
//    yuv.x = rgb.r *  0.2126  + rgb.g *  0.7152  + rgb.b *  0.0722;
//    yuv.y = rgb.r * -0.09991 + rgb.g * -0.33609 + rgb.b *  0.436;
//    yuv.z = rgb.r *  0.615   + rgb.g * -0.55861 + rgb.b * -0.05639;
//    yuv.yz += 1.0;
//    yuv.yz *= 0.5;
//    return yuv;
//}
//vec3 yuv2rgb(vec3 yuv) {
//    yuv.yz /= 0.5;
//    yuv.yz -= 1.0;
//    vec3 rgb = vec3(0.);
//    rgb.r = yuv.x +                    yuv.z *  1.28033;
//    rgb.g = yuv.x + yuv.y * -0.21482 + yuv.z * -0.38059;
//    rgb.b = yuv.x + yuv.y *  2.12798;
//    return clamp(rgb, 0.0, 1.0);
//}
//

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
    let x = modulo(x + t_up, 1.);
    return x / t_up * step(x, t_up) +
           (1. - x) / (1. - t_up) * (1. - step(x, t_up));
}

//// Box from [0, 0] to (1, 1)
//float box(vec2 p) {
//    vec2 b = step(0., p) - step(1., p);
//    return b.x * b.y;
//}
//

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
    let i = floor(p);
    let x = fract(p);
    // x = .5*(1.-cos(M_PI*x));
    let x = 3. * x * x - 2. * x * x * x;
    let a = rand(i + 0.);
    let b = rand(i + 1.);
    return mix(a, b, x);
}

fn noise2(p: vec2<f32>) -> f32 {
    let ij = floor(p);
    let xy = fract(p);
    // xy = .5*(1.-cos(M_PI*xy));
    let xy = 3. * xy * xy - 2. * xy * xy * xy;
    let a = rand2((ij+vec2<f32>(0.,0.)));
    let b = rand2((ij+vec2<f32>(1.,0.)));
    let c = rand2((ij+vec2<f32>(0.,1.)));
    let d = rand2((ij+vec2<f32>(1.,1.)));
    let x1 = mix(a, b, xy.x);
    let x2 = mix(c, d, xy.x);
    return mix(x1, x2, xy.y);
}

fn noise3(p: vec3<f32>) -> f32 {
    let ijk = floor(p);
    let xyz = fract(p);
    // xyz = .5*(1.-cos(M_PI*xyz));
    let xyz = 3. * xyz * xyz - 2. * xyz * xyz * xyz;
    let a = rand3((ijk+vec3<f32>(0.,0.,0.)));
    let b = rand3((ijk+vec3<f32>(1.,0.,0.)));
    let c = rand3((ijk+vec3<f32>(0.,1.,0.)));
    let d = rand3((ijk+vec3<f32>(1.,1.,0.)));
    let e = rand3((ijk+vec3<f32>(0.,0.,1.)));
    let f = rand3((ijk+vec3<f32>(1.,0.,1.)));
    let g = rand3((ijk+vec3<f32>(0.,1.,1.)));
    let h = rand3((ijk+vec3<f32>(1.,1.,1.)));
    let x1 = mix(a, b, xyz.x);
    let x2 = mix(c, d, xyz.x);
    let y1 = mix(x1, x2, xyz.y);
    let x3 = mix(e, f, xyz.x);
    let x4 = mix(g, h, xyz.x);
    let y2 = mix(x3, x4, xyz.y);
    return mix(y1, y2, xyz.z);
}

fn noise4(p: vec4<f32>) -> f32 {
    let ijkl = floor(p);
    let xyzw = fract(p);
    // xyz = .5*(1.-cos(M_PI*xyz));
    let xyzw = 3. * xyzw * xyzw - 2. * xyzw * xyzw * xyzw;
    let a = rand4((ijkl+vec4<f32>(0.,0.,0.,0.)));
    let b = rand4((ijkl+vec4<f32>(1.,0.,0.,0.)));
    let c = rand4((ijkl+vec4<f32>(0.,1.,0.,0.)));
    let d = rand4((ijkl+vec4<f32>(1.,1.,0.,0.)));
    let e = rand4((ijkl+vec4<f32>(0.,0.,1.,0.)));
    let f = rand4((ijkl+vec4<f32>(1.,0.,1.,0.)));
    let g = rand4((ijkl+vec4<f32>(0.,1.,1.,0.)));
    let h = rand4((ijkl+vec4<f32>(1.,1.,1.,0.)));
    let i = rand4((ijkl+vec4<f32>(0.,0.,0.,1.)));
    let j = rand4((ijkl+vec4<f32>(1.,0.,0.,1.)));
    let k = rand4((ijkl+vec4<f32>(0.,1.,0.,1.)));
    let l = rand4((ijkl+vec4<f32>(1.,1.,0.,1.)));
    let m = rand4((ijkl+vec4<f32>(0.,0.,1.,1.)));
    let n = rand4((ijkl+vec4<f32>(1.,0.,1.,1.)));
    let o = rand4((ijkl+vec4<f32>(0.,1.,1.,1.)));
    let q = rand4((ijkl+vec4<f32>(1.,1.,1.,1.)));
    let x1 = mix(a, b, xyzw.x);
    let x2 = mix(c, d, xyzw.x);
    let y1 = mix(x1, x2, xyzw.y);
    let x3 = mix(e, f, xyzw.x);
    let x4 = mix(g, h, xyzw.x);
    let y2 = mix(x3, x4, xyzw.y);
    let z1 = mix(y1, y2, xyzw.z);

    let x5 = mix(i, j, xyzw.x);
    let x6 = mix(k, l, xyzw.x);
    let y3 = mix(x5, x6, xyzw.y);
    let x7 = mix(m, n, xyzw.x);
    let x8 = mix(o, q, xyzw.x);
    let y4 = mix(x7, x8, xyzw.y);
    let z2 = mix(y3, y4, xyzw.z);
    return mix(z1, z2, xyzw.w);
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

//float onePixel = 1. / min(iResolution.x, iResolution.y);
//

//#line 1
