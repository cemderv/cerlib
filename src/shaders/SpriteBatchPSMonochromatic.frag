uniform sampler2D SpriteImage;

in vec4 cer_v2f_Color;
in vec2 cer_v2f_UV;

out vec4 out_Color;

void main() {
    float texValue = texture(SpriteImage, cer_v2f_UV).x;
    out_Color = vec4(1.0, 1.0, 1.0, texValue) * cer_v2f_Color;
}

