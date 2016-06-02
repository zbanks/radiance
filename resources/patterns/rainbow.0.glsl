#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iChannel[2];
uniform sampler2D iFrame;
uniform float iIntensity;
uniform float iAudioHi;
uniform float iAudioMid;
uniform float iAudioLow;

#define M_PI 3.1415926535897932384626433832795

// Nice routines from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    float deviation;
    if(iIntensity < 0.25) {
        deviation = iIntensity * cos(M_PI * mod(iTime, 2.) - 1.);
    } else { 
        deviation = iTime * (iIntensity - 0.25) * (1. / 0.75);
    }

    vec3 hsv = rgb2hsv(gl_FragColor.rgb);
    hsv.r = mod(hsv.r + 1. + deviation, 1.);
    gl_FragColor.rgb = hsv2rgb(hsv);
}
