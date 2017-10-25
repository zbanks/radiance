#property description Apply black outline around edges
#property author https://www.shadertoy.com/view/XssGD7 + zbanks

void main()
{
	// Sobel operator
	float off = onePixel;
	vec3 o = vec3(-off, 0.0, off);
	vec4 gx = vec4(0.0);
	vec4 gy = vec4(0.0);
	vec4 t;
	gx += texture(iInput, uv + o.xz);
	gy += gx;
	gx += 2.0*texture(iInput, uv + o.xy);
	t = texture(iInput, uv + o.xx);
	gx += t;
	gy -= t;
	gy += 2.0*texture(iInput, uv + o.yz);
	gy -= 2.0*texture(iInput, uv + o.yx);
	t = texture(iInput, uv + o.zz);
	gx -= t;
	gy += t;
	gx -= 2.0*texture(iInput, uv + o.zy);
	t = texture(iInput, uv + o.zx);
	gx -= t;
	gy -= t;
	vec4 grad = sqrt(gx * gx + gy * gy);

    float black = clamp(1.0 - length(grad) * 0.9, 0., 1.);
    black = pow(black, mix(1.0, 2.0, iIntensity));

    vec4 newColor = texture(iInput, uv);
    newColor.rgb *= mix(1.0, black, smoothstep(0.0, 0.5, iIntensity));
    fragColor = newColor;
}
