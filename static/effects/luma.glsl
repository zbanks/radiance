#property description Makes black transparent (luma keying)

void main(void) {
    fragColor = texture(iInput, uv);

    float luma = 1. - (1. - fragColor.r) * (1. - fragColor.g) * (1. - fragColor.b);
    float a_out = luma / max(0.001, (iIntensity - 0.1) / 0.9); // Full black always becomes transparent, but this curve sets how almost-black things behave
    a_out = max(a_out, 1. - 10. * iIntensity); // For the first 10%, fade from identity to luma-keyed image (makes the transition more continuous and pleasing)
    a_out = min(a_out, fragColor.a); // Never get more opaque than the original image
    fragColor.a = a_out;
}
