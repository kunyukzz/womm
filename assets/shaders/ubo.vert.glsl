// Vertex GLSL
#version 420
layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec2 in_TexCoord;

layout(set = 0, binding = 0) uniform MVP {
    mat4 projection;
	mat4 view;
} mvp;

layout(push_constant) uniform Push {
    mat4 model;
} pc;

layout(location = 1) out struct DTO {
	vec2 texcoord;
} out_dto;

void main() {
	out_dto.texcoord = in_TexCoord;
	gl_Position = mvp.projection * mvp.view * pc.model * vec4(in_Pos, 1.0);
}
