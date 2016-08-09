uniform sampler2D iTexture;
in vec2 v_uv;
out layout(location = 0) vec4 f_color0;

void main(void) {
    f_color0 = texture2D(iTexture, (v_uv ) );
}
