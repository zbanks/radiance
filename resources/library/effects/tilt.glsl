#property description Move input vertically

void main()
{
    vec2 pt = uv;
    pt.y = mod(pt.y + iIntensity * pow(defaultPulse, 2.),1.);
    fragColor = texture(iInputs[0], pt);
}
