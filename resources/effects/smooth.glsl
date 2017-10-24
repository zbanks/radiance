#property description Apply gaussian resampling ( at pixel aligned points )
float squared(float x) { return x * x;}
float gaussian(float x, float sigma)
{
    return ((sigma!=0) ? exp(-0.5*squared(x/sigma)) : ((x==0) ? 1.0 : 0.0));
}

void main()
{
    float sigma = iIntensity * 16.;
    vec4 acc = vec4(0.,0.,0.,0);
    float norm = 0.;
    float stp = 1./iResolution.y;
    for(float i = -16.; i <= 16.; i+=1.) {
        float off =i * stp;
        vec2 pt = clamp(vec2(uv.x,uv.y + off),0.,1.);

        float k = gaussian(i,sigma);
        norm += k;
        acc += k * texture(iChannel[1],pt);
    }
    fragColor = acc / norm;
}
#buffershader
float squared(float x) { return x * x;}
float gaussian(float x, float sigma)
{
    return ((sigma!=0) ? exp(-0.5*squared(x/sigma)) : ((x==0) ? 1.0 : 0.0));
}

void main()
{
    float sigma = iIntensity * 16.;
    vec4 acc = vec4(0.,0.,0.,0);
    float norm = 0.;
    float stp = 1./iResolution.x;
    for(float i = -16.; i <= 16.; i+=1.) {
        float off = stp * i;
        vec2 pt = clamp(vec2(uv.x + off,uv.y),0.,1.);
        float k = gaussian(i,sigma);
        norm += k;
        acc += k * texture(iInputs[0],pt);
    }
    fragColor = acc / norm;
}
