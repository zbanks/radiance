#version 130

uniform bool iLeftOnTop;
uniform bool iSelection;
uniform float iAudioHi;
uniform float iAudioLow;
uniform float iAudioMid;
uniform float iAudioLevel;
uniform float iIntensity;
uniform float iIntensityIntegral;
uniform float iTime;
uniform float iFPS;
uniform int iBins;
uniform int iLeftDeckSelector;
uniform int iLength;
uniform int iPatternIndex;
uniform int iRightDeckSelector;
uniform int iSelected;
uniform int iIndicator;
uniform sampler1D iSpectrum;
uniform sampler1D iWaveform;
uniform sampler1D iBeats;
uniform sampler2D iChannel[3];
uniform sampler2D iFrame;
uniform sampler2D iFrameLeft;
uniform sampler2D iFrameRight;
uniform sampler2D iPreview;
uniform sampler2D iStrips;
uniform sampler2D iTexture;
uniform sampler2D iText;
uniform sampler2D iName;
uniform vec2 iPosition;
uniform vec2 iResolution;
uniform vec2 iTextResolution;
uniform vec2 iNameResolution;

#define M_PI 3.1415926535897932384626433832795

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

vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
}

float rounded_rect_df(vec2 center, vec2 size, float radius) {
    return length(max(abs(gl_FragCoord.xy - center) - size, 0.0)) - radius;
}

vec3 dataColor(ivec3 data) {
    return vec3(data) / vec3(255.);
}

float inBox(vec2 coord, vec2 bottomLeft, vec2 topRight) {
    vec2 a = step(bottomLeft, coord) - step(topRight, coord);
    return a.x * a.y;
}

float smoothBox(vec2 coord, vec2 bottomLeft, vec2 topRight, float width) {
    vec2 a = smoothstep(bottomLeft, bottomLeft + vec2(width), coord) - smoothstep(topRight - vec2(width), topRight, coord);
    return min(a.x, a.y);
}

float rand(float c){
    return fract(sin(c * 12.9898) * 43758.5453);
}

float rand(vec2 c){
    return fract(sin(dot(c, vec2(12.9898,78.233))) * 43758.5453);
}

float rand(vec3 c){
    return fract(sin(dot(c, vec3(12.9898,78.233, 52.942))) * 43758.5453);
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

float sawtooth(float x, float t_up) {
    x = mod(x + t_up, 1.);
    return x / t_up * step(x, t_up) +
           (1. - x) / (1 - t_up) * (1. - step(x, t_up));
}

#line 0
