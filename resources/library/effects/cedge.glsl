#property description Derivative of https://www.shadertoy.com/view/XssGD7, with tighter edges.

vec4 get_texture(vec2 off, vec2 cor) {
    return texture(iInput, uv + off * cor);
}

void main()
{
	// Sobel operator
    float off = 8. * iIntensity;
    vec2  cor = 1. / iResolution.xy;
	vec3 o = vec3(-off, 0.0, off);
	vec4 gx = vec4(0.0);
	vec4 gy = vec4(0.0);
	vec4 t;
	gx += get_texture(o.xz,cor);
	gy += gx;
	gx += 2.0*get_texture(o.xy,cor);
	t = get_texture(o.xx,cor);
	gx += t;
	gy -= t;
	gy += 2.0*get_texture(o.yz,cor);
	gy -= 2.0*get_texture(o.yx,cor);
	t = get_texture(o.zz,cor);
	gx -= t;
	gy += t;
	gx -= 2.0*get_texture(o.zy,cor);
	t = get_texture(o.zx,cor);
	gx -= t;
	gy -= t;
	vec4 grad = sqrt(gx * gx + gy * gy);
    grad.xyz /= sqrt(off);
    grad.a = max(max(grad.r, grad.g), max(grad.b, grad.a));

    vec4 original = texture(iInput, uv);
    if(iIntensity > 0.) {
        fragColor = grad;
    }else{
        fragColor = original;
    }
}
