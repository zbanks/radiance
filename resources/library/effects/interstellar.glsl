#property description Lightspeed travel rays
#property author https://www.shadertoy.com/view/Xdl3D2

const float tau = 6.28318530717958647692;

// Gamma correction
#define GAMMA (2.2)

vec3 ToLinear( in vec3 col )
{
	// simulate a monitor, converting colour values into light values
	return pow( col, vec3(GAMMA) );
}

vec3 ToGamma( in vec3 col )
{
	// convert back into colour values, so the correct light will come out of the monitor
	return pow( col, vec3(1.0/GAMMA) );
}

vec4 Noise( in ivec2 x )
{
	return 2. * texture(iNoise, (vec2(x)+0.5)/256.0, -100.);
}

void main()
{
	vec3 ray;
	ray.xy = 2.0*(gl_FragCoord.xy-iResolution.xy*.5)/iResolution.x;
	ray.z = 1.0;

	//float offset = iTime*.5;
	//float speed2 = (cos(offset)+1.0)*2.0;
	float offset = iIntensityIntegral * 0.5;
	float speed2 = 3. * iAudioLow;
	float speed = speed2+.1;
	//offset += sin(offset)*.96;
	//offset *= 2.0;


	vec3 col = vec3(0);

	vec3 stp = ray/max(abs(ray.x),abs(ray.y));

	vec3 pos = 2.0*stp+.5;
	for ( int i=0; i < 20; i++ )
	{
		float z = Noise(ivec2(pos.xy)).x;
		z = fract(z-offset);
		float d = 50.0*z-pos.z;
		float w = pow(max(0.0,1.0-8.0*length(fract(pos.xy)-.5)),2.0);
		vec3 c = max(vec3(0),vec3(1.0-abs(d+speed2*.5)/speed,1.0-abs(d)/speed,1.0-abs(d-speed2*.5)/speed));
		col += 1.5*(1.0-z)*c*w;
		pos += stp;
	}

	vec4 fc;
    fc.rgb = ToGamma(col);
    fc.a = max(max(fragColor.r, fragColor.g), fragColor.b);
    vec4 c = texture(iInput, uv);
    fragColor = composite(c, fc * iIntensity);
}
