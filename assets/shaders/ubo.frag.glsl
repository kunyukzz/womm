// Fragment GLSL
#version 420
layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform Push {
	mat4 model;
	vec4 diffuse_color;
} pc;

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

layout(location = 1) in struct DTO {
	vec3 normal;
	vec2 texcoord;
} in_dto;

void main() {
	vec3 lightDir = normalize(vec3(0.5, 0.5, 0.5));
	vec3 normal = normalize(in_dto.normal);

	float ambient = 0.2;
	float diff = max(dot(normal, lightDir), 0.0);
	float intensity = ambient + (diff * 1.0);

	vec4 texColor = texture(diffuse_sampler, in_dto.texcoord);
	out_Color = pc.diffuse_color * texColor * intensity;

	// raw normal
	//out_Color = vec4(normal * 0.5 + 0.5, 1.0);

	//out_Color = vec4(1.0, 0.0, 1.0, 1.0);
	//out_Color = pc.diffuse_color * texture(diffuse_sampler, in_dto.texcoord);
}
