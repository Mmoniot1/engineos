//By Monica Moniot
#version 330 core
layout (location = 0) in vec2 pos_in;
layout (location = 1) in vec4 color_in;
layout (location = 2) in vec2 tex_in;
uniform mat3 projection;

out vec2 texture_pos;
out vec4 color;

void main() {
	gl_Position = vec4((projection*vec3(pos_in, 1.0)).xy, 0.0, 1.0);
	color = color_in;
	texture_pos = tex_in;
}
//|End of file
