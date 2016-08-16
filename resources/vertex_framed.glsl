#version 430

uniform layout(location = 0) vec2 u_global_size;
in layout(location = 0) vec2 a_corner;
in layout(location = 1) vec2 a_size;
in layout(location = 2) float a_layer;
out flat vec2 vg_corner;
out flat vec2 vg_size;
out flat float vg_layer;
void main()
{
    vg_corner = a_corner;
    vg_size   = a_size;
    vg_layer  = a_layer;
}
