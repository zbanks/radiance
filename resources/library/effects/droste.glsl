#property description Droste effect (spiral forever!)
#property author http://roy.red/droste-.html + zbanks


// Complex functions from http://glslsandbox.com/e#5664.1

vec2 cconj(const vec2 c) { return vec2(c.x, -c.y); }

vec2 cmul(const vec2 c1, const vec2 c2)
{
	return vec2(
		c1.x * c2.x - c1.y * c2.y,
		c1.x * c2.y + c1.y * c2.x
	);
}

vec2 cdiv(const vec2 c1, const vec2 c2)
{
	return cmul(c1, cconj(c2)) / dot(c2, c2);
}

vec2 clog (const vec2 z)
{
	return vec2 (log(length(z)), atan(z.y, z.x));
}

vec2 circle (float a) { return vec2 (cos(a), sin(a)); }

vec2 cexp (const vec2 z)
{
	return circle(z.y) * exp(z.x);
}

float r1 = 0.1;
float r2 = 2.0;
vec2 droste(vec2 z) {
    // 4. Take the tiled strips back to ordinary space.
    z = clog(z);
    // 3. Scale and rotate the strips
    float scale = log(r2/r1);
    // Negate the angle to twist the other way
    float angle = atan(scale/(2.0*M_PI));
    z = cdiv(z, cexp(vec2(0,angle))*cos(angle)); 
    // 2. Tile the strips
    z.x += iTime / 4.;
    z.x = mod(z.x,scale);
    // 1. Take the annulus to a strip
    z = cexp(z)*r1;
    z /= r2 * 2.0;
    return z;
}

float f(float x,float n){
    return pow(n,-floor(log(x)/log(n)));
}
vec2 droste2(vec2 z) {
    float ratio = 5.264;
    float angle = atan(log(ratio)/(2.0*M_PI));
    z = cexp(cdiv(clog(z), cexp(vec2(0,angle))*cos(angle)));
    vec2 a_z = abs(z);
    z *= f(max(a_z.x,a_z.y)*2.,ratio);
    return z / ratio;
}

void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;
    vec2 newUV = droste(normCoord);
    newUV = newUV / aspectCorrection + 0.5;

    fragColor = texture(iInput, mix(uv, newUV, iIntensity));
}
