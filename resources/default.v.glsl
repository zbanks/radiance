#version 130 core

#extension ARB_explicit_attrib_location : enable

uniform vec2 u_global_size = vec2(1.,1.);
in layout(location = 0) vec2 a_position;
in layout(location = 1) vec2 a_corner;
in layout(location = 2) vec2 a_size;
in layout(location = 3) vec2 a_texcoord;

out vec2 v_texcoord;
out vec2 v_fragcoord;
out vec2 flat v_corner;
out vec2 flat v_size;
void main()
{
    v_fragcoord = (a_position - a_corner) / a_size;
    v_texcoord  = a_texcoord;
    v_corner    = a_corner;
    v_size      = a_size;
    vec2 fractional_position = a_position / u_globa_size;
    gl_Position = vec4(fractional_position * 2. - 1., 0, 1.);
}
