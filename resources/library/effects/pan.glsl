#property description Move input horizontally

void main()
{
    vec2 pt = uv;
    pt.x = mod(pt.x + iIntensity * pow(defaultPulse, 2.), 1.);
    fragColor = texture(iInputs[0], pt);
}
