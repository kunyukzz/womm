#version 420
layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec3 in_Color;
layout(location = 0) out vec3 frag_Color;

layout(set = 0, binding = 0) uniform MVP {
    mat4 projection;
	mat4 view;
} mvp;

layout(push_constant) uniform Push {
    mat4 model;
} pc;

void main() {
	gl_Position = mvp.projection * mvp.view * pc.model * vec4(in_Pos, 1.0);
	frag_Color = in_Color;
}
