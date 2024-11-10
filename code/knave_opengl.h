#ifndef KNAVE_OPENGL_H
#define KNAVE_OPENGL_H

#include <GL/gl.h>

#include "knave.h"

typedef void(*Kgl_Debug_Callback)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param);

#define KGL_FUNCTIONS \
	X(void, glAttachShader, (GLuint program, GLuint shader)) \
	X(void, glBindBuffer, (GLenum target, GLuint buffer)) \
	X(void, glBindVertexArray, (GLuint array)) \
	X(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage)) \
	X(void, glCompileShader, (GLuint shader)) \
	X(GLuint, glCreateProgram, (void)) \
	X(GLuint, glCreateShader, (GLenum type)) \
	X(void, glDeleteProgram, (GLuint program)) \
	X(void, glDeleteShader, (GLuint shader)) \
	X(void, glEnableVertexAttribArray, (GLint location)) \
	X(void, glGenBuffers, (GLsizei n, GLuint *buffers)) \
	X(void, glGenVertexArrays, (GLsizei n, GLuint *arrays)) \
	X(GLint, glGetAttribLocation, (GLuint program, const GLchar *name)) \
	X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint *params)) \
	X(void, glGetShaderInfoLog, (GLuint shader, GLsizei max_length, GLsizei *length, GLchar *info_log)) \
	X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params)) \
	X(void, glGetProgramInfoLog, (GLuint program, GLsizei max_length, GLsizei *length, GLchar *info_log)) \
	X(void, glLinkProgram, (GLuint program)) \
	X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar **string, const GLint *length)) \
	X(void, glUseProgram, (GLuint program)) \
	X(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)) \

#define X(RETURN, FUNCTION, ARGUMENTS) typedef RETURN(*K_FN(FUNCTION))ARGUMENTS;
KGL_FUNCTIONS
#undef X

#ifdef KGL_HOST
#   define X(RETURN, FUNCTION, ARGUMENTS) K_FN(FUNCTION) FUNCTION;
#else
#   define X(RETURN, FUNCTION, ARGUMENTS) extern K_FN(FUNCTION) FUNCTION;
#endif
KGL_FUNCTIONS
#undef X

#define KGL_CALL(EXPRESSION) \
	do \
	{ \
		EXPRESSION; \
		kgl_check_error(__FILE__, __LINE__); \
	} \
	while (0)

typedef struct Kgl_Shader
{
	GLuint id;
}
Kgl_Shader;

typedef struct Kgl_Program
{
	GLuint id;
}
Kgl_Program;

void kgl_check_error(const char *file_name, int line_number);
bool kgl_program_from_strings(Kgl_Program *program, const char *vertex_string, const char *fragment_string);

#endif // KNAVE_OPENGL_H
