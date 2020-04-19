varying highp vec2 uv;
uniform sampler2D iTexture;
void main() {
    gl_FragColor = texture2D(iTexture, uv);
}
