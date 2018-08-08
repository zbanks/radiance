#property description Rotating squares
#property inputCount 2
#property frequency 1

void main(void) {
    fragColor = texture(iInput, uv);
    float x = iIntensity;
    // Dead width 
    float dw = 0.22;

    // TODO: what's the right value for '0.3'?
    //float scale = pow(32.0, iIntensity) * 0.3;
    float scale = 4.0;
    float t = iTime * iFrequency * 0.25;

    float tw;
    float td = modf(t * 4.0, tw);
    td = tw + smoothstep(0., 1., td);
    td /= 2.0;
    // t with detent
    //t = mix(t, td, 0.97);

    float q = step(mod(t + 0.25, 1.0), 0.5);
    q = min(q, step(0.5 - dw * 0.5, x));
    q = max(q, step(0.5 + dw * 0.5, x));

    float theta = t * M_PI;
    vec2 uvNorm = (uv - 0.5) * scale * aspectCorrection;
    vec2 r1 = mod(uvNorm, 1.0);
    vec2 r2 = mod(uvNorm + vec2(0.5), 1.0);
    vec2 rep = mix(r1, r2, q);
    vec2 off = (rep - vec2(0.5, 0.5)) * 2.0;

    float s = sin(theta);
    float c = cos(theta);
    mat2 rot = mat2(c, -s, s, c);
    vec2 xy = off * rot;
    float eps = 0.05;
    float slope = (1.0 + dw / 2.0);
    float d = 1.0 / sqrt(2.0) * min(1.0, 2.0 * ((1.0 + dw / 2.0) * (0.5 - abs(x - 0.5))));
    float v = smoothstep(d - eps, d + eps, max(abs(xy.x), abs(xy.y)));

    vec4 left = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 right = vec4(0.0, 0.0, 0.0, 1.0);
    
    left = texture(iInputs[0], uv);
    right = texture(iInputs[1], uv);

    fragColor = mix(right, left, abs(q - v));
}
