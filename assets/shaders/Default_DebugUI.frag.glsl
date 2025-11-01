// UI Fragment Shader
#version 420
layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform Push {
	mat4 model;
	vec4 diffuse_color;
} pc;

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

layout(location = 1) in struct DTO {
	vec2 tex_coord;
} in_dto;

void main() {
	//out_Color = vec4(1.0, 1.0,1.0, 1.0);
	out_Color = pc.diffuse_color * texture(diffuse_sampler, in_dto.tex_coord);
}
