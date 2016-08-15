#version 430

uniform layout(location=1) sampler2D u_texture;

in vec2 v_uv;
out vec4 f_color0;

void main()
{
    f_color0 = vec4(1.,1.,1.,texture2D(u_texture, v_uv).r);
}
