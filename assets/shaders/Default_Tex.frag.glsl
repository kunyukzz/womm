// Fragment GLSL - Default_Tex
#version 420
layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform Push {
	mat4 model;
	vec4 diffuse_color;
	uint texture_index;
} pc;

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler[3];

layout(location = 1) in struct DTO {
	vec3 normal;
	vec2 texcoord;
} in_dto;

void main() {
	vec3 lightDir = normalize(vec3(0.8, 0.6, 0.2));
	vec3 normal = normalize(in_dto.normal);

	float ambient = 0.2;
	float diff = dot(normal, lightDir);
	float lightlevel = diff > 0.0 ? 0.8 : 0.4;
	float intensity = ambient + (diff * 0.85);

	vec4 texColor = texture(diffuse_sampler[pc.texture_index], in_dto.texcoord);
	out_Color = pc.diffuse_color * texColor * intensity;
}
