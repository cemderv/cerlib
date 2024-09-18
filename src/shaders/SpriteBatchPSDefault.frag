uniform sampler2D SpriteImage;

in vec4 cer_v2f_Color;
in vec2 cer_v2f_UV;

out vec4 out_Color;

void main() {
    out_Color = texture(SpriteImage, cer_v2f_UV) * cer_v2f_Color;
}

