#version 430

layout(points) in;
layout(triangle_strip, max_vertices=4) out;
uniform layout(location = 0) vec2 u_global_size;
in vec2 vg_corner[];
in vec2 vg_size  [];

out flat vec2 v_corner;
out flat vec2 v_size;
out vec2      v_uv;

void main()
{
    vec2 size   = vg_size[0]  * 2/ u_global_size;
    vec2 corner = vg_corner[0] * 2 / u_global_size - vec2(1);

    {
        v_corner = vg_corner[0];
        v_size   = vg_size[0];
        v_uv     = vec2(0,0);
        gl_PrimitiveID = gl_PrimitiveIDIn;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_corner = vg_corner[0];
        v_size   = vg_size[0];
        v_uv     = vec2(0,1);
        gl_PrimitiveID = gl_PrimitiveIDIn;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_corner = vg_corner[0];
        v_size   = vg_size[0];
        v_uv     = vec2(1,0);
        gl_PrimitiveID = gl_PrimitiveIDIn;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    {
        v_corner = vg_corner[0];
        v_size   = vg_size[0];
        v_uv     = vec2(1,1);
        gl_PrimitiveID = gl_PrimitiveIDIn;
        gl_Position = vec4(corner + size * v_uv,0,1);
        EmitVertex();
    }
    EndPrimitive();
}
