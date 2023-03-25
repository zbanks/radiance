#property description 3D cube
#property frequency 1

// From maze.glsl
fn viewMatrix(eye: vec3<f32>, dir: vec3<f32>, up: vec3<f32>) -> mat4x4<f32> {
    // Based on gluLookAt man page
    let f = normalize(dir);
    let s = normalize(cross(f, up));
    let u = cross(s, f);
    return mat4x4<f32>(
        vec4<f32>(s, 0.0),
        vec4<f32>(u, 0.0),
        vec4<f32>(-f, 0.0),
        vec4<f32>(0.0, 0.0, 0.0, 1.0)
    );
}

// Draw a copy of the input filling the given quad
fn draw(bottomLeft: vec2<f32>, bottomRight: vec2<f32>, topLeft: vec2<f32>, topRight: vec2<f32>, uv: vec2<f32>) -> vec4<f32> {
    let ssi = smoothstep(0., 0.2, iIntensity);

    // Four-point projection taken from https://math.stackexchange.com/a/339033
    // Step 1: Compute b in a * e = d
    let a = mat3x3<f32>(vec3<f32>(bottomLeft, 1.),
                        vec3<f32>(bottomRight, 1.),
                        vec3<f32>(topLeft, 1.));
    let d = vec3(topRight, 1.);
    // TODO inverse can be replaced with adjugate
    let e = inverse3(a) * d;

    // Step 2: Scale matrix a by e
    let a = mat3x3<f32>(
        a[0] * e.x,
        a[1] * e.y,
        a[2] * e.z,
    );

    // Step 3: Compute the matrix B
    // which is a constant, so just write it down
    let b = mat3x3<f32>(0., 0., -1.,
                        1., 0.,  1.,
                        0., 1.,  1.);

    // Step 4, 5: Get the combined matrix transform c
    // TODO inverse can be replaced with adjugate
    let c = b * inverse3(a);

    // Step 6: Project the point uv using transform c
    let h = c * vec3((uv - 0.5) * mix(vec2(1.), aspectCorrection, ssi), 1.);

    // Step 7: De-homogenize
    let newUV = h.xy / h.z;

    // Crop the output square if its not
    let squareUV = (newUV - 0.5) / aspectCorrection + 0.5;

    return textureSample(iInputsTex[0], iSampler, mix(newUV, squareUV, ssi)) * box(newUV);
}

fn main(uv: vec2<f32>) -> vec4<f32> {
    let ssi = smoothstep(0., 0.4, iIntensity);

    var cubePoints = array<vec3<f32>, 8>(
        vec3<f32>(-1., -1.,  1.),
        vec3<f32>( 1., -1.,  1.),
        vec3<f32>(-1.,  1.,  1.),
        vec3<f32>( 1.,  1.,  1.),
        vec3<f32>(-1., -1., -1.),
        vec3<f32>( 1., -1., -1.),
        vec3<f32>(-1.,  1., -1.),
        vec3<f32>( 1.,  1., -1.)
    );

    // Indices into cubePoints
    var faces = array<vec4<i32>, 6>(
        vec4<i32>(0, 1, 2, 3), // Front
        vec4<i32>(1, 5, 3, 7), // Right
        vec4<i32>(2, 3, 6, 7), // Top
        vec4<i32>(5, 4, 7, 6), // Back
        vec4<i32>(4, 0, 6, 2), // Left
        vec4<i32>(4, 5, 0, 1) // Bottom
    );

    // Perspective
    // TBH I don't actually know how perspective works
    let p = -0.2 * ssi;

    // Zoom
    let z = mix(0.5, 0.2, sqrt(ssi));

    let xf = mat4x4<f32>(z,  0., 0., 0.,
                         0., z,  0., 0.,
                         0., 0., z,  p,
                         0., 0., 0., 1.);

    let t = iIntensityIntegral * iFrequency + 1.;
    let eye = vec3<f32>(sin(t), sin(0.5 * t), cos(t));
    let eye = mix(vec3<f32>(0., 0., 1.), eye, ssi);
    // TODO inverse can be replaced with adjugate
    let xf = xf * inverse4(viewMatrix(eye, -eye, vec3<f32>(0., 1., 0.)));

    // Use the transformation matrix xf to project cubePoints
    for (var i: i32 = 0; i < 8; i++) {
        let h = xf * vec4(cubePoints[i], 1.);
        let cubePointXY = h.xy / h.w;
        // Don't project z, we will use that for occlusion checking
        let cubePointZ = h.z;
        cubePoints[i] = vec3<f32>(cubePointXY, cubePointZ);
    }

    // From https://stackoverflow.com/a/749206
    // BUBBLE SORT
    // so that we draw in the right order
    for (var n: i32 = 5; n != 0; n--) {
        for(var i: i32 = 0; i < n; i++) {
            // Compare the center of each cube face
            if cubePoints[faces[i].x].z + cubePoints[faces[i].w].z
            >  cubePoints[faces[i+1].x].z + cubePoints[faces[i+1].w].z {
                let tmp = faces[i];
                faces[i] = faces[i+1];
                faces[i+1] = tmp;
            }
        }
    }

    // Draw the six faces
    var c = vec4<f32>(0.);

    for (var i: i32 = 0; i < 6; i++) {
        let face = faces[i];
        let adj = select(1., smoothstep(0., 0.1, iIntensity), i == 0);
        c = composite(c, draw(cubePoints[face.x].xy, cubePoints[face.y].xy, cubePoints[face.z].xy, cubePoints[face.w].xy, uv) * adj);
    }
    return c;
}
