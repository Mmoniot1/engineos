#version 330 core

// in vec3 color;
// in vec2 texture_pos;

out vec4 out_color;

uniform vec4 color;

void main()
{
	out_color = color;
}
//|End of file
