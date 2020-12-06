#version 450

const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));

layout(location = 0) out vec2 uv;

void main() {
    vec2 vertex = varray[gl_VertexIndex];
    gl_Position = vec4(vertex,0.,1.);
    uv = 0.5 * (vertex + 1.);
}
