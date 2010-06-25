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

#include "GLvideo_rt.h"
#include "GLvideo_renderer.h"
#include "GLvideo_params.h"
#include "GLutil.h"

#include "colourMatrix.h"

#include "videoTransport.h"
#include "videoData.h"

#include "stats.h"
#define addStatPerfFloat(name, val) addStat("OpenGL", name, val)
#define addStatPerfInt(name, val) addStatUnit("OpenGL", name, val, "ms")

#define DEBUG 0

static void updateShaderVars(int program, VideoData *videoData, float colour_matrix[4][4]);

void GLvideo_rt::render()
{
	QTime perfTimer; //performance timer for measuring indivdual processes during rendering

	VideoData* videoData = vt->getFrame();
	if (!videoData) {
		return;
	}

	bool videoData_is_new_frame = false;
	if (lastframenum != videoData->frameNum) {
		videoData_is_new_frame = true;
		lastframenum = videoData->frameNum;
	}

	//perform any format conversions
	perfTimer.start();
	switch (videoData->renderFormat) {

	case VideoData::V210:
		//check for v210 data - it's very difficult to write a shader for this
		//due to the lack of GLSL bitwise operators, and it's insistance on filtering the textures
		if (DEBUG)
			printf("Converting V210 frame\n");
		videoData->convertV210();
		break;

	case VideoData::V16P4:
	case VideoData::V16P2:
	case VideoData::V16P0:
		//convert 16 bit data to 8bit
		//this avoids endian-ness issues between the host and the GPU
		//and reduces the bandwidth to the GPU
		if (DEBUG)
			printf("Converting 16 bit frame\n");
		videoData->convertPlanar16();
		break;

	default:
		//no conversion needed
		break;
	}
	addStatPerfInt("ReadData", perfTimer.elapsed());

	bool createGLTextures = false;
	//check for video dimensions changing
	if ((lastsrcwidth != videoData->Ywidth)
		|| (lastsrcheight != videoData->Yheight)
		|| (lastisplanar != videoData->isPlanar)) {
		if (DEBUG)
			printf("Changing video dimensions to %dx%d\n",
				   videoData->Ywidth, videoData->Yheight);
		createGLTextures = true;

		lastsrcwidth = videoData->Ywidth;
		lastsrcheight = videoData->Yheight;
		lastisplanar = videoData->isPlanar;
	}

	if (videoData->isPlanar)
		currentShader = shaderPlanar;
	else
		currentShader = shaderUYVY;

	if (params.deinterlace)
		currentShader |= Deinterlace;
	else
		currentShader &= ~Deinterlace;

	if (createGLTextures) {
		if (DEBUG)
			printf("Creating GL textures\n");
		renderer->createTextures(videoData);
	}

	if (params.matrix_valid == false) {
		buildColourMatrix(colour_matrix, params);
		params.matrix_valid = true;
	}

	//update the uniform variables in the fragment shader
	perfTimer.restart();
	updateShaderVars(programs[currentShader], videoData, colour_matrix);
	addStatPerfInt("UpdateVars", perfTimer.elapsed());

	/* setup viewport for rendering (letter/pillarbox) */
	aspectBox(videoData, displaywidth, displayheight, !params.aspect_ratio_lock);

	if (params.deinterlace) {
		glUseProgramObjectARB(programs[currentShader]);
		int i = glGetUniformLocationARB(programs[currentShader], "field");
		glUniform1fARB(i, (float)videoData->fieldNum);
		glUseProgramObjectARB(0);
	}

	perfTimer.restart();
	/* only upload new frames (old ones stay in the GL) */
	if(videoData_is_new_frame) {
		renderer->uploadTextures(videoData);
	}
	addStatPerfInt("Upload", perfTimer.elapsed());

	perfTimer.restart();
		renderer->renderVideo(videoData, programs[currentShader]);
	addStatPerfInt("RenderVid", perfTimer.elapsed());
}

static void
updateShaderVars(int program, VideoData *videoData, float colour_matrix[4][4])
{
	if (DEBUG)
		printf("Updating fragment shader variables\n");

	glUseProgramObjectARB(program);
	//data about the input file to the shader
	int i;
	i = glGetUniformLocationARB(program, "CHsubsample");
	glUniform1fARB(i, (float)(videoData->Ywidth / videoData->Cwidth));

	i = glGetUniformLocationARB(program, "CVsubsample");
	glUniform1fARB(i, (float)(videoData->Yheight / videoData->Cheight));

	i = glGetUniformLocationARB(program, "colour_matrix");
	glUniformMatrix4fvARB(i, 1, true, (float*) colour_matrix);

	glUseProgramObjectARB(0);
}
