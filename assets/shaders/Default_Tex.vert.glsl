// Vertex GLSL - Default_Tex
#version 420
layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

layout(set = 0, binding = 0) uniform MVP {
    mat4 projection;
	mat4 view;
} mvp;

layout(push_constant) uniform Push {
    mat4 model;
	vec4 diffuse_color;
	uint texture_index;
} pc;

layout(location = 1) out struct DTO {
	vec3 normal;
	vec2 texcoord;
} out_dto;

void main() {
	out_dto.texcoord = in_TexCoord;
	mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
	out_dto.normal = normalize(normalMatrix * in_Normal);
	gl_Position = mvp.projection * mvp.view * pc.model * vec4(in_Pos, 1.0);
}
