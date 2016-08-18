#version 430

in flat vec2 v_corner;
in flat vec2 v_size;
in      vec2 v_uv;
in flat float v_layer;
out layout(location = 0) vec4 f_color0;

// Time, measured in beats. Wraps around to 0 every 16 beats, [0.0, 16.0)
uniform layout(location = 1) float iTime;
// Audio levels, high/mid/low/level, [0.0, 1.0]
uniform layout(location = 2) vec4 iAudio;

#define iAudioLow iAudio.x
#define iAudioMid iAudio.y
#define iAudioHi  iAudio.z
#define iAudioLevel iAudio.w

// Intensity slider, [0.0, 1.0]

uniform layout(location = 3) float iIntensity;

// Intensity slider integrated with respect to wall time mod 1024, [0.0, 1024.0)
uniform layout(location = 4) float iIntensityIntegral;

// (Ideal) output rate in frames per second
uniform layout(location = 5) float iFPS;

// Output of the previous pattern
uniform layout(location = 6) sampler2D iFrame;

// Previous outputs of the other channels (e.g. foo.1.glsl) 
uniform layout(location = 7) sampler2D iChannel[3];

uniform layout(location = 10) sampler2DArray iAllPatterns;
uniform layout(location = 11) int  iPatternIndex;
#define M_PI 3.1415926535897932384626433832795

//
// The following is only used for UI; not for patterns
//
uniform bool iLeftOnTop;
uniform bool iSelection;
uniform int  iBins;
uniform int  iLeftDeckSelector;
uniform int  iLength;
uniform int  iRightDeckSelector;
uniform int  iSelected;
uniform int  iIndicator;

uniform sampler1D iSpectrum;
uniform sampler1D iWaveform;
uniform sampler1D iBeats;
uniform sampler2D iFrameLeft;
uniform sampler2D iFrameRight;
uniform sampler2D iPreview;
uniform sampler2D iStrips;
uniform sampler2D iTexture;
uniform sampler2D iText;
uniform sampler2D iName;

// Utilities to convert from an RGB vec3 to an HSV vec3
vec3 rgb2hsv(vec3 c);
vec3 hsv2rgb(vec3 c);
// Alpha-compsite two colors, putting one on top of the other
vec4 composite(vec4 under, vec4 over);
// Sawtooth wave
float sawtooth(float x, float t_up);
// Predictable randomness
float rand(float c);
float rand(vec2 c);
float rand(vec3 c);
float noise(float p);
float noise(vec2 p);
float noise(vec3 p);
float rounded_rect_df(vec2 coord, vec2 center, vec2 size, float radius);
vec3 dataColor(ivec3 data);
float inBox(vec2 coord, vec2 bottomLeft, vec2 topRight);
float smoothBox(vec2 coord, vec2 bottomLeft, vec2 topRight, float width);
vec4 fancy_rect(vec2,vec2,vec2,bool);
const float RADIUS=25.;
const vec2  PAT_SIZE = vec2(45., 75.);

float rounded_rect_df(vec2 coord,vec2 center, vec2 size, float radius) {
    return length(max(abs(coord- center) - size, 0.0)) - radius;
}
vec4 fancy_rect(vec2 coord, vec2 center, vec2 size, bool selected) {
    vec4 c;
    vec4 color;

    if(selected) {
        float highlight_df = rounded_rect_df(coord,center, size, RADIUS - 10.);
        color = vec4(1., 1., 0., 0.5 * (1. - smoothstep(0., 50., max(highlight_df, 0.))));
    } else {
        float shadow_df = rounded_rect_df(coord,center + vec2(10., -10.), size, RADIUS - 10.);
        color = vec4(0., 0., 0., 0.5 * (1. - smoothstep(0., 20., max(shadow_df, 0.))));
    }

    float df = rounded_rect_df(coord,center, size, RADIUS);
    c = vec4(vec3(0.1) * (center.y + size.y + RADIUS - (coord).y) / (2. * (size.y + RADIUS)), clamp(1. - df, 0., 1.));
    color = composite(color, c);
    return color;
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
float sdCapsule( vec2 p, vec2 a, vec2 b, float r )
{
    vec2 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

void glow(vec2 p, inout vec4 color) {
    vec2 LEN = vec2(300., 0.);
    float FRINGE = 75.;
    color = composite(color, vec4(0., 0.5, 1., 0.5 * max(0., 1. - sdCapsule((v_uv*v_size).xy, p - LEN, p + LEN, FRINGE) / FRINGE)));
}


