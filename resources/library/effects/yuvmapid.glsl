#property description Identity input for yuvmap

void main(void) {
    // Y could be anything, use 0.5
    vec2 scaledUV = (uv - 0.5) / 4.0 + 0.5;
    vec3 yuv = vec3(0.5, scaledUV);

    vec4 base = texture(iInput, uv);
    vec4 yuvColor = clamp(vec4(yuv2rgb(yuv), 1.0), 0.0, 1.0);
    fragColor = mix(base, yuvColor, iIntensity * pow(defaultPulse, 2.));
}

