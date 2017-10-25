#property description 3D flying view
#property author inigo quilez (iq/2013), modified by Eric Van Albert
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Modified for radiance by Eric Van Albert

void main()
{
    float an = iTime*0.2;
    vec2 p = mat2(cos(an),-sin(an),sin(an),cos(an)) * (-1. + 2. * uv);
    vec2 puv = vec2(p.x,1.0)/abs(p.y) + vec2(0., 4.) * iIntensityIntegral;
    puv = abs(mod(0.2 * puv, 2.) - 1.);

    puv = mix(uv, puv, smoothstep(0.0, 0.2, iIntensity));
    vec4 c = texture(iInput, puv);
    //c.a *= abs(uv.y * 0.8);
    c.a = mix(c.a, c.a * min(abs(p.y)* 3.8, 1.), smoothstep(0.0, 0.2, iIntensity));
	fragColor = premultiply(c);
}
