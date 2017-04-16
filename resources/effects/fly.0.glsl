// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for radiance by Eric Van Albert

void main()
{
    float an = iTime*0.2;
    vec2 p = mat2(cos(an),-sin(an),sin(an),cos(an)) * (-1. + 2. * uv);
    vec2 puv = vec2(p.x,1.0)/abs(p.y) + vec2(0., 4.) * iIntensityIntegral;
    puv = abs(mod(0.2 * puv, 2.) - 1.);

    vec4 c = texture2D(iFrame, puv);
    //c.a *= abs(uv.y * 0.8);
    c.a *= min(abs(p.y)* 3.8, 1.);
	gl_FragColor = c;
}
