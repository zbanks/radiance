#version 430
uniform layout(location = 0) vec2 u_global_size;
in layout(location = 0) vec2 a_position;

out vec2 v_uv;

void main()
{
    v_uv = a_position / u_global_size;
    gl_Position = vec4((v_uv * 2) - 1, 0, 1);
}
