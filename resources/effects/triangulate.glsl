#property description "Pixelate" the output into triangles

void main(void) {
    vec3 normCoord = vec3(0.0);
    normCoord.xy = (uv - 0.5) * aspectCorrection;
    normCoord.z = normCoord.x + normCoord.y;

    float bs = 256. * pow(2, -9. * iIntensity);
    vec3 bins = vec3(0.0);
    //bins.xy = bs * aspectCorrection;
    bins.xy = vec2(bs);
    bins.z = bins.y;
    //bins.y += bins.x;

    //normCoord.xy = round(normCoord.xy * bins.xy) / bins.xy;
    normCoord = round(normCoord * bins) / bins;
    //normCoord.z = (mod(floor(normCoord.z * bins.z), 2.0) - 1.0) / (bins.z * 2);

    //normCoord.y -= normCoord.x;
    //normCoord.y -= normCoord.z;
    normCoord.y = mix(normCoord.y, normCoord.z, 0.5);

    vec2 newUV = normCoord.xy / aspectCorrection + 0.5;
    newUV = mix(uv, newUV, smoothstep(0., 0.1, iIntensity));

    //fragColor = texture(iInput, newUV);
    fragColor.rgba = vec4(1.0);
    fragColor.rg = newUV.rr;
}
