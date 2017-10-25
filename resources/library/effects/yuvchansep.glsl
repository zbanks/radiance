#property description YUV color channel separation

void main(void) {
    float spin = iTime * 0.2;
    float separate = iIntensity * 0.1 * cos(iTime * M_PI * 0.25);
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    vec2 yOffset = normCoord - separate * vec2(cos(spin), sin(spin));
    vec2 uOffset = normCoord - separate * vec2(cos(2. + spin), sin(2. + spin));
    vec2 vOffset = normCoord - separate * vec2(cos(4. + spin), sin(4. + spin));

    vec4 yImage = texture2D(iInput, yOffset / aspectCorrection + 0.5);
    vec4 uImage = texture2D(iInput, uOffset / aspectCorrection + 0.5);
    vec4 vImage = texture2D(iInput, vOffset / aspectCorrection + 0.5);

    vec3 yuv = vec3(0.);
    yuv.r = rgb2yuv(demultiply(yImage).rgb).r;
    yuv.g = rgb2yuv(demultiply(uImage).rgb).g;
    yuv.b = rgb2yuv(demultiply(vImage).rgb).b;

    // hmmm alpha is hard #TODO
    //vec3 rgb = vec3(yImage.r, uImage.g, vImage.b);
    //float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    
    vec3 rgb = yuv2rgb(yuv);
    //float a_out = 1. - (1. - rgb.r) * (1. - rgb.g) * (1. - rgb.b);
    float a_out = yImage.a;

    gl_FragColor = premultiply(vec4(rgb, a_out));
}
