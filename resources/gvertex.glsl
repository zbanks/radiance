#version 430

uniform layout(location = 0) vec2 u_global_size;
in layout(location = 0)      vec2 a_position;
in layout(location = 1)      vec2 a_uv;

out vec2 v_uv;

void main()
{
    v_uv = a_uv;
    gl_Position = (a_position * 2 / u_global_size) - vec2(1);
}
