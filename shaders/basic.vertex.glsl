#version 330 core

in vec3 position;
in vec3 color;

out vec3 fragment_color;

void main()
{
	gl_Position = vec4(position.xyz, 1.0f);
	fragment_color = color;
}
