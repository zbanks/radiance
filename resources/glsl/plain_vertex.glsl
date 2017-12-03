attribute vec4 vPosition;
varying highp vec2 uv;
void main() {
    gl_Position = vec4(vPosition.xy, 1., 1.);
    uv = vPosition.zw;
}
