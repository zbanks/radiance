// From https://www.shadertoy.com/view/4sXSzs

void main()
{
	vec2 q = (uv - vec2(0.5, 0.5)) * aspectCorrection;

    float t2 = iTime * 0.2;
    vec2 offset = vec2(sin(t2), cos(t2)) * 0.2;
    q -= offset;

	float len = length(q);

    float t = iIntensityIntegral * 3.;
	float a = 6. * atan(q.y, q.x) / (2. * M_PI) + t * 0.3;
	float b = 6. * atan(q.y, q.x) / (2. * M_PI) + t * 0.3;
	float r1 = 0.3 / len + t * 0.5;
	float r2 = 0.2 / len + t * 0.5;

    vec2 texcoords = vec2(a + 0.1 / len, r1);
	vec4 tex1 = texture2D(iInput, abs(mod(texcoords, 2.) - 1.));
    vec4 c = tex1 * smoothstep(0., 0.1, len);
    c *= smoothstep(0.1, 0.2, iIntensity);

    vec2 texcoords2 = (uv - 0.5 - offset) / (1. - smoothstep(0., 0.2, iIntensity)) + 0.5 + offset;
    vec4 c2 = texture2D(iInput, texcoords2);
    c2 *= box(texcoords2);

    gl_FragColor = composite(c2, c);
}

