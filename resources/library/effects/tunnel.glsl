#property description 3D tunnel
#property author https://www.shadertoy.com/view/4sXSzs
#property frequency 1

void main()
{
	vec2 q = (uv - vec2(0.5, 0.5)) * aspectCorrection;

    float t2 = iFrequency * iTime * (M_PI / 16.);
    vec2 offset = vec2(sin(t2), cos(t2)) * iFrequency * 0.1;
    q -= offset;

	float len = length(q);

    float t = iFrequency * iTime;
	float a = 6. * atan(q.y, q.x) / (2. * M_PI) + t * 0.3;
	float b = 6. * atan(q.y, q.x) / (2. * M_PI) + t * 0.3;
	float r1 = 0.3 / len + t * 0.5;
	float r2 = 0.2 / len + t * 0.5;

    vec2 texcoords = vec2(a + 0.1 / len, r1);
    texcoords = abs(mod(texcoords, 2.) - 1.);

    fragColor = texture(iInput, mix(uv, texcoords, iIntensity));

    fragColor *= smoothstep(0., 0.1 * iIntensity, len);
}
