/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2008-2010 BBC Research
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>

#include "GLutil.h"

/* compile the fragment shader specified by @src@.
 * Returns the allocated program object name.
 * Aborts whole program on failure .*/
GLuint compileFragmentShader(const char *src)
{
	/* Compile shader source */
	GLuint shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(shader, 1, (const GLcharARB**)&src, NULL);
	glCompileShaderARB(shader);

	GLint compiled;
	glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
	if (compiled <= 0) {
		GLint length;
		glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		char *s = (char *)malloc(length);
		glGetInfoLogARB(shader, length, &length, s);
		printf("Compile Failed: %s\n", s);
		free(s);
		abort();
	}

	/* Create a program by linking the compiled shader  */
	GLuint program = glCreateProgramObjectARB();
	glAttachObjectARB(program, shader);
	glLinkProgramARB(program);

	GLint linked;
	glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
	if (linked <= 0) {
		GLint length;
		glGetObjectParameterivARB(program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		char *s = (char *)malloc(length);
		glGetInfoLogARB(program, length, &length, s);
		printf("Link Failed: %s\n", s);
		free(s);
		abort();
	}

	/* the shader object is not required, unreference it (deletion will
	 * occur when the program object is deleted) */
	glDeleteObjectARB(shader);

	return program;
}
