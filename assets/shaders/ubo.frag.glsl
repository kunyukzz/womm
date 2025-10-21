// Fragment GLSL
#version 420
layout(location = 0) out vec4 out_Color;

layout(set = 1, binding = 0) uniform Push {
	vec4 diffuse_color;
} pc;

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

layout(location = 1) in struct DTO {
	vec2 texcoord;
} in_dto;

void main() {
	//out_Color = vec4(frag_Color, 1.0);
	//out_Color = pc.diffuse_color; // using color
	out_Color = pc.diffuse_color * texture(diffuse_sampler, in_dto.texcoord);
}
