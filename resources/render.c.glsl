#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(std430) readonly buffer Points {
    vec2 v_data[];
};

layout(std430) writeonly buffer Values {
    vec4 c_data[];
};
uniform layout(location = 0) sampler2D u_texture;
uniform uint                           u_strip_length = 150;
void main()
{
    uint point_index = gl_GlobalInvocationID.y * u_strip_length + gl_GlobalInvocationID.x;
    vec2 point = v_data[point_index];
    c_data[point_index] = texelFetch(u_texture, ivec2(point * textureSize(u_texture,0)),0);
}
