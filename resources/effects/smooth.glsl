// Apply gaussian blur
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
        vec2 pt = vec2(uv.x,uv.y + off);

        float k = gaussian(i,sigma);
        norm += k;
        acc += k * texture2D(iChannel[1],pt);
    }
    gl_FragColor = acc / norm;
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
        vec2 pt = vec2(uv.x + off,uv.y);

        float k = gaussian(i,sigma);
        norm += k;
        acc += k * texture2D(iInputs[0],pt);
    }
    gl_FragColor = acc / norm;
}
