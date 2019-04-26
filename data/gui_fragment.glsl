//By Monica Moniot
#version 330 core
in vec2 texture_pos;
in vec4 color;
uniform sampler2D font_texture;

out vec4 out_color;

void main() {
	out_color = vec4(color.xyz, color.w*texture(font_texture, texture_pos).w);
}
//|End of file
