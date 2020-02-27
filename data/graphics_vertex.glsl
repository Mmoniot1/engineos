//By Monica Moniot
#version 330 core
layout (location = 0) in vec2 pos_in;
layout (location = 1) in vec2 tex_in;
layout (location = 2) in vec4 color_in;
uniform mat3 projection;


out vec4 color;
out vec2 texture_pos;

void main() {
	gl_Position = vec4(projection*vec3(pos_in, 1.0), 1.0);
	color = color_in;
	texture_pos = tex_in;
}
//|End of file
