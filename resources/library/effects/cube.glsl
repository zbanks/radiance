#property description 3D cube

// From maze.glsl
mat4 viewMatrix(vec3 eye, vec3 dir, vec3 up) {
    // Based on gluLookAt man page
    vec3 f = normalize(dir);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return mat4(
        vec4(s, 0.0),
        vec4(u, 0.0),
        vec4(-f, 0.0),
        vec4(0.0, 0.0, 0.0, 1)
    );
}

// Draw a copy of the input filling the given quad
vec4 draw(vec2 bottomLeft, vec2 bottomRight, vec2 topLeft, vec2 topRight) {
    // Four-point projection taken from https://math.stackexchange.com/a/339033
    // Step 1: Compute b in a * e = d
    mat3 a = mat3(vec3(bottomLeft, 1.),
                  vec3(bottomRight, 1.),
                  vec3(topLeft, 1.));
    vec3 d = vec3(topRight, 1.);
    // TODO inverse can be replaced with adjugate
    vec3 e = inverse(a) * d;

    // Step 2: Scale matrix a by e
    a[0] *= e.x;
    a[1] *= e.y;
    a[2] *= e.z;

    // Step 3: Compute the matrix B
    // which is a constant, so just write it down
    mat3 b = mat3(0., 0., -1.,
                  1., 0.,  1.,
                  0., 1.,  1.);

    // Step 4, 5: Get the combined matrix transform c
    // TODO inverse can be replaced with adjugate
    mat3 c = b * inverse(a);

    // Step 6: Project the point uv using transform c
    vec3 h = c * vec3((uv - 0.5) * aspectCorrection, 1.);

    // Step 7: De-homogenize
    vec2 newUV = h.xy / h.z;

    // Crop the output square if its not
    vec2 squareUV = (newUV - 0.5) / aspectCorrection + 0.5;

    return texture(iInput, squareUV) * box(newUV);
}

void main() {
    float ssi = smoothstep(0., 0.4, iIntensity);

    vec3 cubePoints[8] = vec3[](
        vec3(-1., -1.,  1.),
        vec3( 1., -1.,  1.),
        vec3(-1.,  1.,  1.),
        vec3( 1.,  1.,  1.),
        vec3(-1., -1., -1.),
        vec3( 1., -1., -1.),
        vec3(-1.,  1., -1.),
        vec3( 1.,  1., -1.)
    );

    // Indices into cubePoints
    ivec4 faces[6] = ivec4[](
        ivec4(0, 1, 2, 3), // Front
        ivec4(1, 5, 3, 7), // Right
        ivec4(2, 3, 6, 7), // Top
        ivec4(5, 4, 7, 6), // Back
        ivec4(4, 0, 6, 2), // Left
        ivec4(4, 5, 0, 1) // Bottom
    );

    // Perspective
    // TBH I don't actually know how perspective works
    float p = -0.2 * ssi;

    // Zoom
    float z = mix(0.5, 0.2, sqrt(ssi));

    mat4 xf = mat4(z,  0., 0., 0.,
                   0., z,  0., 0.,
                   0., 0., z,  p,
                   0., 0., 0., 1.);

    float t = iIntensityIntegral * 2.;
    vec3 eye = vec3(sin(t), sin(0.5 * t), cos(t));
    eye = mix(vec3(0., 0., 1.), eye, ssi);
    // TODO inverse can be replaced with adjugate
    xf = xf * inverse(viewMatrix(eye, -eye, vec3(0., 1., 0.)));

    // Use the transformation matrix xf to project cubePoints
    for(int i=0; i<8; i++) {
        vec4 h = xf * vec4(cubePoints[i], 1.);
        cubePoints[i].xy = h.xy / h.w;
        cubePoints[i].z = h.z; // Don't project z, we will use that for occlusion checking
    }

    // From https://stackoverflow.com/a/749206
    // BUBBLE SORT
    // so that we draw in the right order
    for(int n = 5; n != 0; n--) {
        for(int i = 0; i < n; i++) {
            // Compare the center of each cube face
            if(cubePoints[faces[i].x].z + cubePoints[faces[i].w].z
            >  cubePoints[faces[i+1].x].z + cubePoints[faces[i+1].w].z) {
                ivec4 tmp = faces[i];
                faces[i] = faces[i+1];
                faces[i+1] = tmp;
            }
        }
    }


    // Draw the six faces
    fragColor = vec4(0.);

    for(int i=0; i<6; i++) {
        ivec4 face = faces[i];
        fragColor = composite(fragColor, draw(cubePoints[face.x].xy, cubePoints[face.y].xy, cubePoints[face.z].xy, cubePoints[face.w].xy));
    }
}
