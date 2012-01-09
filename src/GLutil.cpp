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

#include "videoData.h"
#include "GLutil.h"

#include <algorithm>
using namespace std;

/* compile the fragment shader specified by @src@.
 * Returns the allocated program object name.
 * Aborts whole program on failure .*/
unsigned compileFragmentShader(const char *src)
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

/**
 * setCoordSystem:
 *
 * set the co-ordinate system to match the integer coordinates of
 * video_data's geometry.
 */
static void setCoordSystem(unsigned video_data_width, unsigned video_data_height)
{
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, video_data_width, 0, video_data_height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

/**
 * aspectBox:
 *
 * Letter-/Pillar- box the viewport to retain displaying videoData in
 * the correct aspect ratio for the display.
 *
 * NB, currently assumes the pixel_aspect_ratio of the source and
 * destination are identical
 */
void
aspectBox(unsigned video_data_width
         ,unsigned video_data_height
         ,unsigned display_xoff
         ,unsigned display_yoff
         ,unsigned display_width
         ,unsigned display_height
         ,bool force_display_aspect
         ,bool zoom_1to1)
{
	float source_aspect = (float)video_data_width / video_data_height;
	float display_aspect = (float)display_width / display_height;

	/* set full viewport for drawing of black bars */
	glViewport(display_xoff, display_yoff, display_width, display_height);

	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, display_width, 0, display_height, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	if (source_aspect == display_aspect || force_display_aspect) {
		setCoordSystem(video_data_width, video_data_height);
		return;
	}

	int viewport_x = 0;
	int viewport_y = 0;
	unsigned viewport_w = display_width;
	unsigned viewport_h = display_height;

	unsigned w_or_x;
	unsigned y_or_h;

	if (zoom_1to1) {
		/* place video in middle of display, and postagestamp */
		int display_centre_x = display_width / 2;
		int display_centre_y = display_height / 2;
		int video_centre_x = video_data_width / 2;
		int video_centre_y = video_data_height / 2;
		/* if video narrower than display: we need to shrink the viewport
		 * if       wider,                 we limit to the dispay width */
		viewport_w = min(video_data_width, display_width);
		viewport_h = min(video_data_height, display_height);
		viewport_x = max(0, display_centre_x - video_centre_x);
		viewport_y = max(0, display_centre_y - video_centre_y);

		/* draw letterbox */
		glBegin(GL_QUADS);
			glColor3f(0.5f, 0.5f, 0.5f);
			/* bottom */
			glVertex2i(0, 0);
			glVertex2i(display_width, 0);
			glVertex2i(display_width, viewport_y);
			glVertex2i(0, viewport_y);
			/* top */
			glVertex2i(display_width, display_height);
			glVertex2i(0, display_height);
			glVertex2i(0, display_height - viewport_y - 1);
			glVertex2i(display_width, display_height - viewport_y -1);
		glEnd();

		/* configure to draw pillarboxes */
		w_or_x = viewport_x;
		y_or_h = display_height;
	}
	else if (display_aspect > source_aspect) {
		/* pillar boxing required */
		viewport_w = (int)(display_height * source_aspect);
		viewport_x = (display_width - viewport_w) / 2;
		w_or_x = viewport_x;
		y_or_h = viewport_h;
	}
	else {
		/* letter boxing required */
		viewport_h = (int)(display_width / source_aspect);
		viewport_y = (display_height - viewport_h) / 2;
		w_or_x = viewport_w;
		y_or_h = viewport_y;
	}
	/* draw letter/pillar boxes */
	glBegin(GL_QUADS);
		glColor3f(0.5f, 0.5f, 0.5f);
		/* bottom / left */
		glVertex2i(0, 0);
		glVertex2i(w_or_x, 0);
		glVertex2i(w_or_x, y_or_h);
		glVertex2i(0, y_or_h);
		/* top / right */
		glVertex2i(display_width, display_height);
		glVertex2i(display_width - w_or_x -1, display_height);
		glVertex2i(display_width - w_or_x -1, display_height - y_or_h);
		glVertex2i(display_width, display_height - y_or_h);
	glEnd();
	/* set viewport to required area for video rendering */
	glViewport(display_xoff+viewport_x, display_yoff+viewport_y, viewport_w, viewport_h);

	if (zoom_1to1) {
		int xoff = max(0, ((int)video_data_width - (int)display_width)/2);
		int yoff = max(0, ((int)video_data_height - (int)display_height)/2);
		int width = xoff ? display_width : video_data_width;
		int height = yoff ? display_height : video_data_height;
		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0,width,0,height,-1, 1);
		glMatrixMode(GL_MODELVIEW);
	}
	else {
		setCoordSystem(video_data_width, video_data_height);
	}
}
