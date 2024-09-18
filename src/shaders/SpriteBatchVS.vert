uniform mat4 Transformation;

in vec4 vsin_Position;
in vec4 vsin_Color;
in vec2 vsin_UV;

// !!!
// Keep this in sync with GLSLShaderGenerator fragment shader input stage!
// !!!
out vec4 cer_v2f_Color;
out vec2 cer_v2f_UV;

void main() {
    gl_Position = Transformation * vsin_Position;
    cer_v2f_Color = vsin_Color;
    cer_v2f_UV = vsin_UV;
}
