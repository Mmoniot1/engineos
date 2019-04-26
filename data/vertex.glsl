#version 330 core

layout (location = 0) in vec2 pos;
// layout (location = 1) in vec texture_pos_in;
// layout (location = 2) in vec2 aTexCoord;

// out vec3 color;
// out vec2 texture_pos;

uniform mat3 projection;

void main()
{
    gl_Position = vec4(projection*vec3(pos, 1.0), 1.0);
}
//|End of file
