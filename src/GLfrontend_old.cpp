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

#include <QTime>
#include <stdio.h>

#include <GL/glew.h>

#include "GLfrontend_old.h"
#include "GLvideo_renderer.h"
#include "GLvideo_tradtex.h"
#include "GLvideo_pbotex.h"
#include "GLvideo_params.h"
#include "GLutil.h"
#include "shaders.h"

#include "colourMatrix.h"

#include "videoTransport.h"
#include "videoData.h"

#include "stats.h"
#define addStatPerfFloat(name, val) addStat("OpenGL", name, val)
#define addStatPerfInt(name, val) addStatUnit("OpenGL", name, val, "ms")

#define DEBUG 0

static void updateShaderVars(int program, VideoData *video_data, float colour_matrix[4][4]);

using namespace GLvideo;

enum ShaderPrograms {
	Progressive = 0x0,
	Deinterlace = 0x1,
	shaderUYVY = 0,
	shaderPlanar = 2,
	/* Increment in steps of 2 */
	shaderBogus,
	shaderMax
};

GLfrontend_old::GLfrontend_old(GLvideo_params& params, VideoTransport* vt)
	: init_done(0)
	, vt(vt)
	, params(params)
	, doResize(0)
{
	return;
}

GLfrontend_old::~GLfrontend_old() {
	if (renderer) delete renderer;
}

void GLfrontend_old::init() {
	programs[shaderPlanar | Progressive] = compileFragmentShader(shaderPlanarProSrc);
	programs[shaderPlanar | Deinterlace] = compileFragmentShader(shaderPlanarDeintSrc);
	programs[shaderUYVY | Progressive] = compileFragmentShader(shaderUYVYProSrc);
	programs[shaderUYVY | Deinterlace] = compileFragmentShader(shaderUYVYDeintSrc);

	renderer = new GLVideoRenderer::PboTex();

	lastsrcwidth = 0;
	lastsrcheight = 0;
	lastisplanar = false;
	lastframenum = ULONG_MAX;
	currentShader = 0;
}

void GLfrontend_old::resizeViewport(int width, int height)
{
	displaywidth = width;
	displayheight = height;
	doResize = true;
}

void GLfrontend_old::render()
{
	if (!init_done) {
		/* this must be called in a thread with the GL context */
		init();
		init_done = true;
	}

	/* obtain the current frame.  NB, this may be the same frame as last time */
	/* xxx, this will eventually be moved into an action */
	VideoData* video_data = vt->getFrame();
	if (!video_data) {
		return;
	}

	if (video_data->data.packing_format == V210) {
		/* convert v210 to uyvy */
		/* todo: refactor this into a shader */
		convertV210toUYVY(video_data->data);
	}
	else
	if (video_data->data.packing_format == V216) {
		/* convert 16bit data to 8bit */
		/* todo: refactor into a shader */
		/* this avoids endian-ness issues between the host and the GPU
		 * and reduces the bandwidth to the GPU */
		convertV216toUYVY(video_data->data);
	}
	else
	if (video_data->data.packing_format == V16P) {
		convertPlanar16toPlanar8(video_data->data);
	}

	VideoDataOld videoData(video_data);

	unsigned video_data_width = video_data->data.plane[0].width;
	unsigned video_data_height = video_data->data.plane[0].height;

	bool video_data_is_new_frame = false;
	if (lastframenum != video_data->frame_number) {
		video_data_is_new_frame = true;
		lastframenum = video_data->frame_number;
	}

	QTime perfTimer; //performance timer for measuring indivdual processes during rendering
	perfTimer.start();

	bool createGLTextures = false;
	//check for video dimensions changing
	if ((lastsrcwidth != video_data_width)
	|| (lastsrcheight != video_data_height))
	{
		if (DEBUG)
			printf("Changing video dimensions to %dx%d\n", video_data_width, video_data_height);
		createGLTextures = true;

		lastsrcwidth = video_data_width;
		lastsrcheight = video_data_height;
	}

	if (video_data->data.packing_format == V8P
	||  video_data->data.packing_format == V16P) {
		currentShader = shaderPlanar;
	}
	else
		currentShader = shaderUYVY;

	if (params.deinterlace)
		currentShader |= Deinterlace;
	else
		currentShader &= ~Deinterlace;

	if (createGLTextures) {
		if (DEBUG)
			printf("Creating GL textures\n");
		renderer->createTextures(&videoData);
	}

	if (params.matrix_valid == false) {
		buildColourMatrix(colour_matrix, params);
		params.matrix_valid = true;
	}

	//update the uniform variables in the fragment shader
	perfTimer.restart();
	updateShaderVars(programs[currentShader], video_data, colour_matrix);
	addStatPerfInt("UpdateVars", perfTimer.elapsed());

	/* setup viewport for rendering (letter/pillarbox) */
	aspectBox(video_data, displaywidth, displayheight, !params.aspect_ratio_lock);

	if (params.deinterlace) {
		glUseProgramObjectARB(programs[currentShader]);
		int i = glGetUniformLocationARB(programs[currentShader], "field");
		glUniform1fARB(i, (float)video_data->is_field1);
		glUseProgramObjectARB(0);
	}

	perfTimer.restart();
	/* only upload new frames (old ones stay in the GL) */
	if(video_data_is_new_frame) {
		renderer->uploadTextures(&videoData);
	}
	addStatPerfInt("Upload", perfTimer.elapsed());

	perfTimer.restart();
		renderer->renderVideo(&videoData, programs[currentShader]);
	addStatPerfInt("RenderVid", perfTimer.elapsed());
}

static void
updateShaderVars(int program, VideoData *video_data, float colour_matrix[4][4])
{
	if (DEBUG)
		printf("Updating fragment shader variables\n");

	float chroma_h_subsample;
	float chroma_v_subsample;
	switch (video_data->data.chroma_format) {
	case Cr420:
		chroma_h_subsample = 2;
		chroma_v_subsample = 2;
		break;
	case Cr422:
		chroma_h_subsample = 2;
		chroma_v_subsample = 1;
		break;
	default:
	case Cr444:
		chroma_h_subsample = 1;
		chroma_v_subsample = 1;
		break;
	}
	glUseProgramObjectARB(program);
	//data about the input file to the shader
	int i;
	i = glGetUniformLocationARB(program, "CHsubsample");
	glUniform1fARB(i, chroma_h_subsample);

	i = glGetUniformLocationARB(program, "CVsubsample");
	glUniform1fARB(i, chroma_v_subsample);

	i = glGetUniformLocationARB(program, "colour_matrix");
	glUniformMatrix4fvARB(i, 1, true, (float*) colour_matrix);

	glUseProgramObjectARB(0);
}
