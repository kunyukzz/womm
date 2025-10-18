#version 420
layout(location = 0) in vec3 frag_Color;
layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform Push {
	mat4 model;
	vec4 diffuse_color; // using color
} pc;

void main() {
	//out_Color = vec4(frag_Color, 1.0);
	out_Color = pc.diffuse_color; // using color
}
