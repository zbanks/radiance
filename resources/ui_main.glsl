#version 130

#define PATTERN(i) \
    bottomLeft = vec2(200 + i * 200, 300); \
    if(insideBox(gl_FragCoord.xy, bottomLeft, patSize) == 1.) { \
        gl_FragColor = texture(iPattern[i], (gl_FragCoord.xy - bottomLeft) / patSize); \
    }

uniform vec2 iResolution;
uniform sampler2D iPattern[8];

// return 1 if v inside the box, return 0 otherwise
float insideBox(vec2 v, vec2 bottomLeft, vec2 size) {
    vec2 s = step(bottomLeft, v) - step(bottomLeft + size, v);
    return s.x * s.y;
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = (1 - uv.y) * 0.2;
    gl_FragColor = vec4(g, g, g, 1.);

    vec2 bottomLeft;
    vec2 patSize = vec2(textureSize(iPattern[0], 0));

    PATTERN(0)
    PATTERN(1)
    PATTERN(2)
    PATTERN(3)
    PATTERN(4)
    PATTERN(5)
    PATTERN(6)
    PATTERN(7)
}
