#include "knave_opengl.h"

static bool kgl_shader_from_string(Kgl_Shader *shader, const char *string, GLenum type)
{	
	GLuint id;
	KGL_CALL(id = glCreateShader(type));
	KGL_CALL(glShaderSource(id, 1, &string, NULL));
	KGL_CALL(glCompileShader(id));

	int success = GL_FALSE;
	KGL_CALL(glGetShaderiv(id, GL_COMPILE_STATUS, &success));
	if (success == GL_FALSE)
	{		
		char info_log[512];
		KGL_CALL(glGetShaderInfoLog(id, sizeof(info_log), NULL, info_log));
		KERROR("Could not compile shader: %.*s", (int)sizeof(info_log), info_log);

		KGL_CALL(glDeleteShader(id));
		return false;
	}

	shader->id = id;
	return true;
}

static void kgl_destroy_shader(Kgl_Shader *shader)
{
	if (shader->id != 0)
	{
		glDeleteShader(shader->id);
		shader->id = 0;
	}
}

bool kgl_program_from_strings(Kgl_Program *program, const char *vertex_string, const char *fragment_string)
{
	Kgl_Shader vertex;
	if (!kgl_shader_from_string(&vertex, vertex_string, GL_VERTEX_SHADER))
	{
		return false;
	}
	
	Kgl_Shader fragment;
	if (!kgl_shader_from_string(&fragment, fragment_string, GL_FRAGMENT_SHADER))
	{
		kgl_destroy_shader(&vertex);
		return false;
	}

	GLuint id;
	KGL_CALL(id = glCreateProgram());
	KGL_CALL(glAttachShader(id, vertex.id));
	KGL_CALL(glAttachShader(id, fragment.id));
	KGL_CALL(glLinkProgram(id));

	kgl_destroy_shader(&vertex);
	kgl_destroy_shader(&fragment);

	int success = GL_FALSE;
	KGL_CALL(glGetProgramiv(id, GL_LINK_STATUS, &success));
	if (success == GL_FALSE)
	{		
		char info_log[512];
		size_t length = sizeof(info_log);
		KGL_CALL(glGetProgramInfoLog(id, sizeof(info_log), NULL, info_log));
		KERROR("Could not link shader program: %.*s", (int)sizeof(info_log), info_log);

		KGL_CALL(glDeleteProgram(id));
		return false;
	}

	program->id = id;
	return true;
}

void kgl_check_error(const char *file_name, int line_number)
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		KERROR("OpenGL error at file %s line %d: #%d.", file_name, line_number, error);
	}
}
