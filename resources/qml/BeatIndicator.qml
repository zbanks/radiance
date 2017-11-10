import QtQuick 2.7
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 100;
    implicitHeight: 100;

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        uniform highp float iTime;

        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return vec4((over.rgb + under.rgb * (1. - over.a)), a_out);
        }

        vec3 sphereNormal(vec2 pt) {
            float exists = (1. - step(1., length(pt)));
            vec3 normal = vec3(pt.x, pt.y, sqrt(max(1. - pt.x * pt.x - pt.y * pt.y, 0.)));
            return normal * exists;
        }

        void main(void) {
            vec4 c = vec4(1., 0., 0., 1.);
            vec2 uvm = uv;
            uvm.y = 1. - uvm.y;

            // Specular exponent
            float shininess = 40.;

            // Specular amount
            float k_spec = 0.5;

            // Diffuse amount
            float k_diff = 0.8;

            // Ambient amount
            float k_amb = 0.2;

            float height = 1. - pow(abs(2. * (mod(iTime, 1.) - 0.5)), 2.);
            height *= (1. - 0.4 * step(1., mod(iTime, 4.)));
            vec3 pt = vec3(7. * (uvm - 0.5) + vec2(0., -height * 3.5 + 1.5), 0.);
            vec3 viewer = vec3(0., 0., 4.);
            vec3 light = vec3(-3., 5., 5.);

            // Blinn-Phong model
            vec3 n = sphereNormal(pt.xy);
            vec3 l = normalize(light - pt);
            vec3 v = normalize(viewer - pt);
            vec3 h = normalize(l + v); // Halfway vector
            float hit = length(n);
            float spec = pow(max(dot(n, h), 0.), shininess) * k_spec; // Specular component
            float diff = max(dot(n, l), 0.) * k_diff; // Diffuse component

            // Compute the shaded color of this pixel
            vec4 ball = min(c * hit * k_amb + c * diff * k_diff + spec, 1.);
            ball.a = hit;

            // Draw the floor
            vec4 floor = vec4(1., 1., 1., 1.);
            mat3 floorArea = inverse(mat3(.4, .2, .5,
                                          0., .2, .2,
                                          0., 0., 1.));
            floor *= smoothstep(0., 0.5, 1. - length((vec3(uvm, 1.) * floorArea).xy));

            // Heckin' fake shadow
            mat3 shadowArea = inverse(mat3(.3, .2, .55,
                                          0., .2, .21,
                                          0., 0., 1.));
            floor.rgb *= 1. - (0.6 * (1. - smoothstep(0.4, 0.6,  length((vec3(uvm - height * 0.3, 1.) * shadowArea).xy))));

            fragColor = composite(floor, ball);
        }"
}
