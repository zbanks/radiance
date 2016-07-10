#version 130

uniform bool iLeftOnTop;
uniform bool iSelection;
uniform float iAudioHi;
uniform float iAudioLow;
uniform float iAudioMid;
uniform float iIntensity;
uniform float iTime;
uniform int iBins;
uniform int iLength;
uniform int iPatternIndex;
uniform int iSelected;
uniform sampler1D iSpectrum;
uniform sampler1D iWaveform;
uniform sampler2D iChannel[2];
uniform sampler2D iFrame;
uniform sampler2D iFrameLeft;
uniform sampler2D iFrameRight;
uniform sampler2D iPreview;
uniform sampler2D iTexture;
uniform sampler2D iText;
uniform vec2 iPosition;
uniform vec2 iResolution;
uniform vec2 iTextResolution;

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
    // Takes in non premultiplied and outputs non-premultiplied!?!
    float a_out = over.a + under.a * (1. - over.a);
    return vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out);
}

vec4 compositeCR(vec4 under, vec4 over) {
    // Takes non-premultiplied and outputs premultiplied?!?
    return vec4(over.rgb * over.a  + under.rgb * under.a * (1. - over.a), over.a + under.a * (1. - over.a));
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



























