//By Monica Moniot
#version 330 core
in vec2 texture_pos;
in vec4 color;
uniform sampler2D main_texture;

out vec4 color_out;

void main() {
	color_out = mix(color, texture(main_texture, texture_pos), .8);
    // color_out = color_out, texture(main_texture, texture_pos), .8);
}
//|End of file
