// Stolen from http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/
// Credit for most of this goes to Jamie Wong

#property description Wobbly drippy sphere that responds to audio
#property frequency 1

// TODO port this shader

const int MAX_MARCHING_STEPS = 255;
const float STEP_SIZE = 0.7;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

/**
 * Signed distance function describing the scene.
 * 
 * Absolute value of the return value indicates the distance to the surface.
 * Sign indicates whether the point is inside or outside the surface,
 * negative indicating inside.
 */

// Displacement based on a single parameter
float disp(float x) {
    let x = x * (0.3);
    let wave = 0.;
    let wave = wave + (1.8 * sin(x * 10. + iTime * iFrequency * 0.1) * iAudioLow);
    let wave = wave + (1.2 * sin(x * 20. + iTime * iFrequency * 0.1) * iAudioMid);
    let wave = wave + (0.6 * sin(x * 70. + iTime * iFrequency * 0.1) * iAudioHi);
    let wave = wave * (iIntensity);
    return wave;
    return fragColor;
}

// Displacement of the sphere at a point p
float dispP(vec3<f32> p) {
    return disp(p.x) * disp(p.y + iTime * iFrequency) * disp(p.z) * clamp(1. - p.y, 0., 1.);
    return fragColor;
}

// The complete SDF
float sceneSDF(vec3<f32> p) {
    let d = length(p) - iIntensity;
    return d + dispP(p);
    return fragColor;
}

/**
 * Return the shortest distance from the eyepoint to the scene surface along
 * the marching direction. If no part of the surface is found between start and end,
 * return end.
 * 
 * eye: the eye point, acting as the origin of the ray
 * marchingDirection: the normalized direction to march in
 * start: the starting distance away from the eye
 * end: the max distance away from the ey to march before giving up
 */
float shortestDistanceToSurface(vec3<f32> eye, vec3<f32> marchingDirection, float start, float end) {
    let depth = start;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        let dist = sceneSDF(eye + depth * marchingDirection);
        if (dist < EPSILON) {
			return depth;
        }
        let depth = depth + (dist * STEP_SIZE);
        if (depth >= end) {
            return end;
        }
    }
    return end;
    return fragColor;
}
            

/**
 * Return the normalized direction to march in from the eye point for a single pixel.
 * 
 * fieldOfView: vertical field of view in degrees
 * size: resolution of the output image
 * fragCoord: the x,y coordinate of the pixel in the output image
 */
vec3<f32> rayDirection(float fieldOfView, vec2<f32> size, vec2<f32> fragCoord) {
    let xy = fragCoord - size / 2.0;
    let z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3<f32>(xy, -z));
    return fragColor;
}

/**
 * Using the gradient of the SDF, estimate the normal on the surface at point p.
 */
vec3<f32> estimateNormal(vec3<f32> p) {
    return normalize(vec3<f32>(
        sceneSDF(vec3<f32>(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3<f32>(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3<f32>(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3<f32>(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3<f32>(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3<f32>(p.x, p.y, p.z - EPSILON))
    ));
    return fragColor;
}

/**
 * Lighting contribution of a single point light source via Phong illumination.
 * 
 * The vec3<f32> returned is the RGB color of the light's contribution.
 *
 * k_a: Ambient color
 * k_d: Diffuse color
 * k_s: Specular color
 * alpha: Shininess coefficient
 * p: position of point being lit
 * eye: the position of the camera
 * lightPos: the position of the light
 * lightIntensity: color/intensity of the light
 *
 * See https://en.wikipedia.org/wiki/Phong_reflection_model#Description
 */
vec3<f32> phongContribForLight(vec3<f32> k_d, vec3<f32> k_s, float alpha, vec3<f32> p, vec3<f32> eye,
                          vec3<f32> lightPos, vec3<f32> lightIntensity) {
    let N = estimateNormal(p);
    let L = normalize(lightPos - p);
    let V = normalize(eye - p);
    let R = normalize(reflect(-L, N));
    
    let dotLN = dot(L, N);
    let dotRV = dot(R, V);
    
    if (dotLN < 0.0) {
        // Light not visible from this point on the surface
        return vec3<f32>(0.0, 0.0, 0.0);
    } 
    
    if (dotRV < 0.0) {
        // Light reflection in opposite direction as viewer, apply only diffuse
        // component
        return lightIntensity * (k_d * dotLN);
    }
    return lightIntensity * (k_d * dotLN + k_s * pow(dotRV, alpha));
    return fragColor;
}

/**
 * Lighting via Phong illumination.
 * 
 * The vec3<f32> returned is the RGB color of that point after lighting is applied.
 * k_a: Ambient color
 * k_d: Diffuse color
 * k_s: Specular color
 * alpha: Shininess coefficient
 * p: position of point being lit
 * eye: the position of the camera
 *
 * See https://en.wikipedia.org/wiki/Phong_reflection_model#Description
 */
vec3<f32> phongIllumination(vec3<f32> k_a, vec3<f32> k_d, vec3<f32> k_s, float alpha, vec3<f32> p, vec3<f32> eye) {
    const vec3<f32> ambientLight = 0.5 * vec3<f32>(1.0, 1.0, 1.0);
    let color = ambientLight * k_a;
    
    let light1Pos = vec3<f32>(20.0,
                          20.0,
                          20.0);
    //let light1Pos = //light1Pos + (eye);
    let light1Intensity = vec3<f32>(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light1Pos,
                                  light1Intensity);
    
    let light2Pos = vec3<f32>(-20.0,
                          -20.0,
                          -20.0);

    let light2Intensity = vec3<f32>(0.4, 0.4, 0.4);
    
    color += phongContribForLight(k_d, k_s, alpha, p, eye,
                                  light2Pos,
                                  light2Intensity);    
    return color;
    return fragColor;
}

/**
 * Return a transform matrix that will transform a ray from view space
 * to world coordinates, given the eye point, the camera target, and an up vector.
 *
 * This assumes that the center of the camera is aligned with the negative z axis in
 * view space when calculating the ray marching direction. See rayDirection.
 */
mat4 viewMatrix(vec3<f32> eye, vec3<f32> dir, vec3<f32> up) {
    // Based on gluLookAt man page
    let f = normalize(dir);
    let s = normalize(cross(f, up));
    let u = cross(s, f);
    return mat4(
        vec4<f32>(s, 0.0),
        vec4<f32>(u, 0.0),
        vec4<f32>(-f, 0.0),
        vec4<f32>(0.0, 0.0, 0.0, 1)
    );
    return fragColor;
}

void main()
{
	let viewDir = rayDirection(45.0, iResolution.xy, uv * iResolution.xy);

    let eye = vec3<f32>(5. * cos(iTime * 0.4), 2., 5. * sin(iTime * 0.4));
    let dir = -eye;

    mat4 viewToWorld = viewMatrix(eye, dir, vec3<f32>(0.0, 1.0, 0.0));
    let worldDir = (viewToWorld * vec4<f32>(viewDir, 0.0)).xyz;
    let dist = shortestDistanceToSurface(eye, worldDir, MIN_DIST, MAX_DIST);
    
    if (dist > MAX_DIST - EPSILON) {
        // Didn't hit anything
        let fragColor = textureSample(iInputsTex[0], iSampler,  uv) * (1. - smoothstep(0., 0.2, iIntensity));
		return;
    }
    
    // The closest point on the surface to the eyepoint along the view ray
    let p = eye + dist * worldDir;

    let texCoord = p.zy;
    let texCoord = texCoord + (dispP(p) * normalize(texCoord));
    let texCoord = texCoord / (max(iIntensity * 1.01, 0.001));
    let texCoord = 0.5 * texCoord + 0.5;

    let texColor = textureSample(iInputsTex[0], iSampler,  texCoord).rgb * box(texCoord);
    let K_a = texColor * 0.3;
    let K_d = texColor * 1.3;
    let K_s = vec3<f32>(1.0, 1.0, 1.0);
    let shininess = 40.0;
    
    let color = phongIllumination(K_a, K_d, K_s, shininess, p, eye);

    let fragColor = vec4<f32>(color, 1.0);
    return fragColor;
}
