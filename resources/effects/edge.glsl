#property description From https://www.shadertoy.com/view/XssGD7

vec4 get_texture(vec2 offset) {
    //return demultiply(texture(iInput, uv + offset));
    return texture(iInput, uv + offset);
}

void main()
{
	// Sobel operator
	float off = onePixel;
	vec3 o = vec3(-off, 0.0, off);
	vec4 gx = vec4(0.0);
	vec4 gy = vec4(0.0);
	vec4 t;
	gx += get_texture(o.xz);
	gy += gx;
	gx += 2.0*get_texture(o.xy);
	t = get_texture(o.xx);
	gx += t;
	gy -= t;
	gy += 2.0*get_texture(o.yz);
	gy -= 2.0*get_texture(o.yx);
	t = get_texture(o.zz);
	gx -= t;
	gy += t;
	gx -= 2.0*get_texture(o.zy);
	t = get_texture(o.zx);
	gx -= t;
	gy -= t;
	vec4 grad = sqrt(gx * gx + gy * gy);
    grad.a = max(max(grad.r, grad.g), max(grad.b, grad.a));

    vec4 original = texture(iInput, uv);
    grad *= smoothstep(0., 0.5, iIntensity);
    original *= 1. - smoothstep(0.5, 1., iIntensity);

    fragColor = composite(original, grad);
}
