/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2008 BBC Research
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

#include <QtCore>
#include <stdio.h>

#include <GL/glew.h>

#ifdef Q_OS_WIN32
# include <GL/wglew.h>
#endif

#ifdef Q_OS_MACX
# include "agl_getproc.h"
#endif
#include "GLvideo_rtAdaptor.h"

#include "GLvideo_rt.h"
#include "GLvideo_renderer.h"
#include "GLvideo_tradtex.h"
#include "GLvideo_pbotex.h"
#include "GLvideo_osd.h"
#include "GLutil.h"

#include "videoData.h"
#include "videoTransport.h"
#include "stats.h"
#include "colourMatrix.h"

#include "GLvideo_params.h"

#include "shaders.h"

#define addStatPerfFloat(name, val) addStat("OpenGL", name, val)
#define addStatPerfInt(name, val) addStatUnit("OpenGL", name, val, "ms")

#define DEBUG 0


GLvideo_rt::GLvideo_rt(GLvideo_rtAdaptor *gl, VideoTransport *vt, GLvideo_params& params) :
	QThread(), gl(gl), vt(vt), params(params)
{
	renderer[0] = new GLVideoRenderer::PboTex();
	renderer[1] = new GLVideoRenderer::PboTex();
	//renderer = new GLVideoRenderer::PboTex();

#ifdef WITH_OSD
	osd = new GLvideo_osd();
#else
	osd = NULL;
#endif
}

GLvideo_rt::~GLvideo_rt()
{
	doRendering = false;
	wait();
	
	if(renderer[0]) delete renderer[0];
	if(renderer[1]) delete renderer[1];
#ifdef WITH_OSD
	if(osd) delete osd;
#endif
}

void GLvideo_rt::resizeViewport(int width, int height)
{
	displaywidth = width;
	displayheight = height;
}

void GLvideo_rt::compileFragmentShaders()
{
	programs[shaderPlanar | Progressive] = compileFragmentShader(shaderPlanarProSrc);
	programs[shaderPlanar | Deinterlace] = compileFragmentShader(shaderPlanarDeintSrc);
	programs[shaderUYVY | Progressive] = compileFragmentShader(shaderUYVYProSrc);
	programs[shaderUYVY | Deinterlace] = compileFragmentShader(shaderUYVYDeintSrc);
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

void GLvideo_rt::run()
{
	if (DEBUG)
		printf("Starting renderthread\n");

	/* initialize the gl windowing adaptor */
	gl->init();

	VideoData *videoData = NULL;

	//monitoring
	QTime fpsIntervalTime; //measures FPS, averaged over several frames
	QTime frameIntervalTime; //measured the total period of each frame
	QTime perfTimer; //performance timer for measuring indivdual processes during rendering
	int fpsAvgPeriod=1;

	//flags and status
	int lastsrcwidth = 0;
	int lastsrcheight = 0;
	bool lastisplanar = false;
	unsigned long lastframenum = ULONG_MAX;

	//declare shadow variables for the thread worker and initialise them
	doRendering = true;
	int currentShader = 0;
	float colour_matrix[4][4];

	GLVideoRenderer::GLVideoRenderer *rendererA = renderer[0];
	GLVideoRenderer::GLVideoRenderer *rendererB = NULL;
	int render_idx = 0;

	GLenum err = glewInit();
	if (GLEW_OK != err)
		qWarning() << "failed to init glew\n";

	if(!GLEW_ARB_shader_objects)
		qWarning() << "opengl implementation does not support shader objects\n";

	if(!GLEW_ARB_pixel_buffer_object)
		qWarning() << "opengl implementation does not support pixel buffer objects\n";

	if(!GLEW_EXT_framebuffer_object)
		qWarning() << "opengl implementation does not support framebuffer objects\n";

#ifdef Q_OS_WIN32
	wglSwapIntervalEXT(1);
#endif
#ifdef Q_OS_MAC
	my_aglSwapInterval(1);
#endif

	/* startup: clear the buffers to stop rubbish being presented
	 * before first video frame is presented */
	glClearColor(0, 0, 0, 0);
	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);

	compileFragmentShaders();

	while (doRendering) {

		bool createGLTextures = false;

		perfTimer.start();

		perfTimer.restart();
		/* request the next frame.  videoData might not change if using repeats
		 * or displaying fields */
		bool is_new_frame_period = vt->advance();
		videoData = vt->getFrame();
		addStatPerfInt("GetFrame", perfTimer.elapsed());

		if (DEBUG)
			printf("videoData=%p\n", videoData);

		bool videoData_is_new_frame = false;
		if (videoData) {
			if (lastframenum != videoData->frameNum) {
				videoData_is_new_frame = true;
				lastframenum = videoData->frameNum;
			}

			//perform any format conversions
			perfTimer.restart();
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
				renderer[0]->createTextures(videoData);
				renderer[1]->createTextures(videoData);
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
				int i = glGetUniformLocationARB(programs[currentShader],
				                                "field");
				glUniform1fARB(i, (float)videoData->fieldNum);
				glUseProgramObjectARB(0);
			}

			perfTimer.restart();
			/* only upload new frames (old ones stay in the GL) */
			if(videoData_is_new_frame) {
				if(rendererA)
					rendererA->uploadTextures(videoData);

				//rotate the renderers for upload and display
				rendererB = renderer[render_idx];
				render_idx = (render_idx) ? 0 : 1;
				rendererA = renderer[render_idx];
			}
			addStatPerfInt("Upload", perfTimer.elapsed());
		}

		perfTimer.restart();
		gl->swapBuffers();
		addStatPerfInt("SwapBuffers", perfTimer.elapsed());

		if (videoData) {
			perfTimer.restart();
			if (rendererB)
				rendererB->renderVideo(videoData, programs[currentShader]);
			addStatPerfInt("RenderVid", perfTimer.elapsed());

#ifdef WITH_OSD
			perfTimer.restart();
			if(osd) osd->render(videoData, params);
			addStatPerfInt("RenderOSD", perfTimer.elapsed());
#endif
		}

		if (is_new_frame_period) {
			//measure overall frame period
			addStatPerfInt("Interval", frameIntervalTime.elapsed());
			frameIntervalTime.restart();

			//calculate FPS
			if (fpsAvgPeriod == 0) {
				int fintvl = fpsIntervalTime.elapsed();
				if (fintvl)
					addStatPerfFloat("RenderRate", 10*1e3/fintvl);
				fpsIntervalTime.start();
			}
			fpsAvgPeriod = (fpsAvgPeriod + 1) %10;
		}

		GLenum error;
		const GLubyte* errStr;
		if ((error = glGetError()) != GL_NO_ERROR) {
			errStr = gluErrorString(error);
			fprintf(stderr, "OpenGL Error: %s\n", errStr);
		}


	}
}
