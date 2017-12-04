#property description Apply gaussian spatial blur
float squared(float x) { return x * x;}
float gaussian(float x)
{
    return exp(-0.5*x*x);
}

void main()
{
    float sigma = iIntensity / 32.;
    vec4 acc = vec4(0.,0.,0.,0.);
    float norm = 0.;
    float stp = aspectCorrection.y * sigma;
    for(float i = -2.; i <= 2.; i+=(1./16.)) {
        float off = i * stp;
        float k = gaussian(i);
        norm += k;
        vec2 pt = clamp(vec2(uv.x,uv.y + off),0.,1.);
        acc += k * texture(iChannel[1],pt);
    }
    fragColor = acc / norm;
}
#buffershader
float squared(float x) { return x * x;}
float gaussian(float x)
{
    return exp(-0.5*x*x);
}

void main()
{
    float sigma = iIntensity / 32.;
    vec4 acc = vec4(0.,0.,0.,0);
    float norm = 0.;
    float stp = aspectCorrection.x * sigma;
    for(float i = -2.; i <= 2.; i+=(1./16.)) {
        float off = i * stp;
        vec2 pt = clamp(vec2(uv.x + off,uv.y),0.,1.);
        float k = gaussian(i);
        norm += k;
        acc += k * texture(iInputs[0],pt);
    }
    fragColor = acc / norm;
}

