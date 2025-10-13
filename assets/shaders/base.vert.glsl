#version 420
layout(location = 0) in vec3 in_Pos;
layout(location = 1) in vec3 in_Color;
layout(location = 0) out vec3 frag_Color;

void main() {
	gl_Position = vec4(in_Pos, 1.0);
	frag_Color = in_Color;
}
