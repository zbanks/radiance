#version 150

in vec4 gl_FragCoord;
in vec2 uv;
out vec4 fragColor;

// Time, measured in beats. Wraps around to 0 every 16 beats, [0.0, 16.0)
uniform highp float iStep;
uniform highp float iTime;

// Audio levels, high/mid/low/level, [0.0, 1.0]
uniform vec4  iAudio;
#define iAudioLow   iAudio.x
#define iAudioMid   iAudio.y
#define iAudioHi    iAudio.z
#define iAudioLevel iAudio.w

// Resolution of the output pattern
uniform vec2 iResolution;

// Intensity slider, [0.0, 1.0]
uniform lowp float iIntensity;

// Intensity slider integrated with respect to wall time mod 1024, [0.0, 1024.0)
uniform float iIntensityIntegral;

// (Ideal) output rate in frames per second
uniform float iFPS;

// Outputs of previous patterns
uniform sampler2D iInputs[];

// Output of the previous pattern.  Alias to iInputs[0]
#define iInput iInputs[0]

// Full frame RGBA noise
uniform sampler2D iNoise;

// Previous outputs of the other channels (e.g. foo.1.glsl)
uniform sampler2D iChannel[3];

#define M_PI 3.1415926535897932384626433832795

float lin_step(float v) {
    return v * iStep * iFPS;
}
float exp_step(float v) {
    return pow(v, iStep * iFPS);
}
// Utilities to convert from an RGB vec3 to an HSV vec3
// 0 <= H, S, V <= 1
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Utilities to convert from an RGB vec3 to a YUV vec3
// 0 <= Y, U, V <= 1 (*not* -1 <= U, V <= 1)
// U is greenish<->bluish; V is bluish<->redish
// https://en.wikipedia.org/wiki/YUV#Full_swing_for_BT.601
vec3 rgb2yuv(vec3 rgb) {
    vec3 yuv = vec3(0.);
    yuv.x = rgb.r *  0.2126  + rgb.g *  0.7152  + rgb.b *  0.0722;
    yuv.y = rgb.r * -0.09991 + rgb.g * -0.33609 + rgb.b *  0.436;
    yuv.z = rgb.r *  0.615   + rgb.g * -0.55861 + rgb.b * -0.05639;
    yuv.yz += 1.0;
    yuv.yz *= 0.5;
    return yuv;
}
vec3 yuv2rgb(vec3 yuv) {
    yuv.yz /= 0.5;
    yuv.yz -= 1.0;
    vec3 rgb = vec3(0.);
    rgb.r = yuv.x +                    yuv.z *  1.28033;
    rgb.g = yuv.x + yuv.y * -0.21482 + yuv.z * -0.38059;
    rgb.b = yuv.x + yuv.y *  2.12798;
    return rgb;
}

// Turn non-premultipled alpha RGBA into premultipled alpha RGBA
vec4 premultiply(vec4 c) {
    return vec4(c.rgb * c.a, c.a);
}

// Turn premultipled alpha RGBA into non-premultipled alpha RGBA
vec4 demultiply(vec4 c) {
    return clamp(vec4(c.rgb / c.a, c.a), vec4(0.), vec4(1.));
}

// Alpha-compsite two colors, putting one on top of the other. Everything is premultipled
vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb + under.rgb * (1. - over.a)), a_out), vec4(0.), vec4(1.));
}

// Sawtooth wave
float sawtooth(float x, float t_up) {
    x = mod(x + t_up, 1.);
    return x / t_up * step(x, t_up) +
           (1. - x) / (1 - t_up) * (1. - step(x, t_up));
}

// Box from [0, 0] to (1, 1)
float box(vec2 p) {
    vec2 b = step(0., p) - step(1., p);
    return b.x * b.y;
}

// Predictable randomness
float rand(float c){
    return fract(sin(c * 12.9898) * 43758.5453);
}

float rand(vec2 c){
    return fract(sin(dot(c, vec2(12.9898,78.233))) * 43758.5453);
}

float rand(vec3 c){
    return fract(sin(dot(c, vec3(12.9898,78.233, 52.942))) * 43758.5453);
}

float rand(vec4 c){
    return fract(sin(dot(c, vec4(12.9898, 78.233, 52.942, 35.291))) * 43758.5453);
}

float noise(float p) {
    float i = floor(p);
    float x = mod(p, 1.);
    // x = .5*(1.-cos(M_PI*x));
    x = 3.*x*x-2.*x*x*x;
    float a = rand(i+0.);
    float b = rand(i+1.);
    return mix(a, b, x);
}

float noise(vec2 p) {
    vec2 ij = floor(p);
    vec2 xy = mod(p, 1.);
    // xy = .5*(1.-cos(M_PI*xy));
    xy = 3.*xy*xy-2.*xy*xy*xy;
    float a = rand((ij+vec2(0.,0.)));
    float b = rand((ij+vec2(1.,0.)));
    float c = rand((ij+vec2(0.,1.)));
    float d = rand((ij+vec2(1.,1.)));
    float x1 = mix(a, b, xy.x);
    float x2 = mix(c, d, xy.x);
    return mix(x1, x2, xy.y);
}

float noise(vec3 p) {
    vec3 ijk = floor(p);
    vec3 xyz = mod(p, 1.);
    // xyz = .5*(1.-cos(M_PI*xyz));
    xyz = 3.*xyz*xyz-2.*xyz*xyz*xyz;
    float a = rand((ijk+vec3(0.,0.,0.)));
    float b = rand((ijk+vec3(1.,0.,0.)));
    float c = rand((ijk+vec3(0.,1.,0.)));
    float d = rand((ijk+vec3(1.,1.,0.)));
    float e = rand((ijk+vec3(0.,0.,1.)));
    float f = rand((ijk+vec3(1.,0.,1.)));
    float g = rand((ijk+vec3(0.,1.,1.)));
    float h = rand((ijk+vec3(1.,1.,1.)));
    float x1 = mix(a, b, xyz.x);
    float x2 = mix(c, d, xyz.x);
    float y1 = mix(x1, x2, xyz.y);
    float x3 = mix(e, f, xyz.x);
    float x4 = mix(g, h, xyz.x);
    float y2 = mix(x3, x4, xyz.y);
    return mix(y1, y2, xyz.z);
}

float noise(vec4 p) {
    vec4 ijkl = floor(p);
    vec4 xyzw = mod(p, 1.);
    // xyz = .5*(1.-cos(M_PI*xyz));
    xyzw = 3.*xyzw*xyzw-2.*xyzw*xyzw*xyzw;
    float a = rand((ijkl+vec4(0.,0.,0.,0.)));
    float b = rand((ijkl+vec4(1.,0.,0.,0.)));
    float c = rand((ijkl+vec4(0.,1.,0.,0.)));
    float d = rand((ijkl+vec4(1.,1.,0.,0.)));
    float e = rand((ijkl+vec4(0.,0.,1.,0.)));
    float f = rand((ijkl+vec4(1.,0.,1.,0.)));
    float g = rand((ijkl+vec4(0.,1.,1.,0.)));
    float h = rand((ijkl+vec4(1.,1.,1.,0.)));
    float i = rand((ijkl+vec4(0.,0.,0.,1.)));
    float j = rand((ijkl+vec4(1.,0.,0.,1.)));
    float k = rand((ijkl+vec4(0.,1.,0.,1.)));
    float l = rand((ijkl+vec4(1.,1.,0.,1.)));
    float m = rand((ijkl+vec4(0.,0.,1.,1.)));
    float n = rand((ijkl+vec4(1.,0.,1.,1.)));
    float o = rand((ijkl+vec4(0.,1.,1.,1.)));
    float q = rand((ijkl+vec4(1.,1.,1.,1.)));
    float x1 = mix(a, b, xyzw.x);
    float x2 = mix(c, d, xyzw.x);
    float y1 = mix(x1, x2, xyzw.y);
    float x3 = mix(e, f, xyzw.x);
    float x4 = mix(g, h, xyzw.x);
    float y2 = mix(x3, x4, xyzw.y);
    float z1 = mix(y1, y2, xyzw.z);

    float x5 = mix(i, j, xyzw.x);
    float x6 = mix(k, l, xyzw.x);
    float y3 = mix(x5, x6, xyzw.y);
    float x7 = mix(m, n, xyzw.x);
    float x8 = mix(o, q, xyzw.x);
    float y4 = mix(x7, x8, xyzw.y);
    float z2 = mix(y3, y4, xyzw.z);
    return mix(z1, z2, xyzw.w);
}

float onePixel = 1. / min(iResolution.x, iResolution.y);
vec2 aspectCorrection = iResolution / min(iResolution.x, iResolution.y);
