#version 420
layout(location = 0) in vec2 frag_Color;
layout(location = 0) out vec4 out_Color;

layout(set = 0, binding = 1) uniform Color {
    vec4 color;
} pc_color;

layout(set = 0, binding = 2) uniform sampler2D tex_Sampler;

void main() {
	vec4 texColor = texture(tex_Sampler, frag_Color);

	if (texColor.a < 0.01) {
        out_Color = pc_color.color;
    } else {
        out_Color = texColor * pc_color.color;
    }
}
