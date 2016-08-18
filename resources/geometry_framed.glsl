#version 430

layout(points) in;
layout(triangle_strip, max_vertices=4) out;
uniform layout(location = 0) vec2 u_global_size;
uniform layout(location = 12) vec2 u_over_size = vec2(1.1,1.1);
in flat vec2 vg_corner[];
in flat vec2 vg_size  [];
in flat float vg_layer [];
out flat float v_layer;
out flat vec2 v_corner;
out flat vec2 v_size;
out vec2      v_uv;

void main()
{
    vec2 size   = vg_size[0]  * 2/ u_global_size;
    vec2 corner = vg_corner[0] * 2 / u_global_size - vec2(1);
    float _layer = vg_layer[0];
    vec2 _corner= vg_corner[0];
    vec2 _size  = vg_size[0];
    {
        v_layer  = _layer;
        v_corner = _corner;
        v_size   = _size;
        v_uv     = vec2(0,0) + ( 1. - u_over_size);
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_layer  = _layer;
        v_corner = _corner;
        v_size   = vg_size[0];
        v_uv     = vec2(1. - u_over_size.x, u_over_size.y) ;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_corner = _corner;
        v_layer  = _layer;
        v_size   = _size;
        v_uv     = vec2(u_over_size.x,1. - u_over_size.y) ;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_layer  = _layer;
        v_corner = _corner;
        v_size   = _size;
        v_uv     = vec2(1,1) * u_over_size;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    EndPrimitive();
}
