#version 330 core

in vec3 fragment_color;

out vec4 output_color;

void main()
{
	output_color = vec4(fragment_color.xyz, 1.0f);
}
