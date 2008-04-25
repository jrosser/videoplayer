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

#include <stdio.h>
#include <sstream>

#include <QtGui>
#include "GLvideo_rt.h"
#include "GLvideo_mt.h"
#include "GLvideo_rt.h"
#include "GLvideo_tradtex.h"
#include "GLvideo_pbotex.h"
#include "GLvideo_osd.h"

#ifdef Q_WS_X11
#include "GLvideo_x11rep.h"
#endif

#include "videoData.h"
#include "videoTransport.h"
#include "frameQueue.h"
#include "stats.h"
#include "util.h"

#include "GLvideo_params.h"

#include "shaders.h"

#ifdef Q_OS_MACX
#include "agl_getproc.h"
#endif

#define DEBUG 0

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl, GLvideo_params& params) :
	QThread(), glw(gl), params(params)
{
	renderer = new GLVideoRenderer::TradTex();
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
	
	if(renderer) delete renderer;
	if(osd) delete osd;
}

void GLvideo_rt::resizeViewport(int width, int height)
{
	displaywidth = width;
	displayheight = height;
	doResize = true;
}

void GLvideo_rt::compileFragmentShaders()
{
	compileFragmentShader(shaderPlanar | Progressive , shaderPlanarProSrc);
	compileFragmentShader(shaderPlanar | Deinterlace , shaderPlanarDeintSrc);
	compileFragmentShader(shaderUYVY | Progressive, shaderUYVYProSrc);
	compileFragmentShader(shaderUYVY | Deinterlace, shaderUYVYDeintSrc);
}

void GLvideo_rt::compileFragmentShader(int n, const char *src)
{
	GLint length = 0; //log length

	shaders[n] = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	if (DEBUG)
		printf("Shader program is %s\n", src);

	glShaderSourceARB(shaders[n], 1, (const GLcharARB**)&src, NULL);
	glCompileShaderARB(shaders[n]);

	glGetObjectParameterivARB(shaders[n], GL_OBJECT_COMPILE_STATUS_ARB,
	                           &compiled[n]);
	glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB,
	                           &length);
	char *s=(char *)malloc(length);
	glGetInfoLogARB(shaders[n], length, &length, s);
	if (DEBUG)
		printf("Compile Status %d, Log: (%d) %s\n", (int)compiled[n], (int)length,
		       (int)length ? s : NULL);
	if (compiled[n] <= 0) {
		printf("Compile Failed: %s\n", s);
		throw /* */;
	}
	free(s);

	/* Set up program objects. */
	programs[n]=glCreateProgramObjectARB();
	glAttachObjectARB(programs[n], shaders[n]);
	glLinkProgramARB(programs[n]);

	/* And print the link log. */
	if (compiled[n]) {
		glGetObjectParameterivARB(programs[n], GL_OBJECT_LINK_STATUS_ARB,
		                           &linked[n]);
		glGetObjectParameterivARB(shaders[n], GL_OBJECT_INFO_LOG_LENGTH_ARB,
		                           &length);
		s=(char *)malloc(length);
		glGetInfoLogARB(shaders[n], length, &length, s);
		if (DEBUG)
			printf("Link Status %d, Log: (%d) %s\n", (int)linked[n], (int)length, s);
		free(s);
	}
}


void GLvideo_rt::updateShaderVars(int program, VideoData *videoData,
                                  GLvideo_params &params, float colour_matrix[4][4])
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

void addStatPerfFloat(char *timer, double d)
{
	Stats &stat = Stats::getInstance();
	std::stringstream ss;
	ss << d;

	stat.addStat("OpenGL", timer, ss.str());
}

void addStatPerfInt(char *timer, int time)
{
	Stats &stat = Stats::getInstance();
	std::stringstream ss;
	ss << time << " " << "ms";

	stat.addStat("OpenGL", timer, ss.str());
}

void GLvideo_rt::run()
{
	if (DEBUG)
		printf("Starting renderthread\n");

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

	//declare shadow variables for the thread worker and initialise them
	doRendering = true;
	int repeat = 0;
	int field = 0;
	int direction = 0;
	int currentShader = 0;
	float colour_matrix[4][4];

	//initialise OpenGL
	glw.makeCurrent();
	const QGLContext* context = glw.context();

	while (!context || !context->isValid()) {
		msleep(50);
		glw.makeCurrent();
		context = glw.context();
	}

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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, displayheight, displaywidth);
	glClearColor(0, 0, 0, 0);

	compileFragmentShaders();

	while (doRendering) {

		bool createGLTextures = false;

		perfTimer.start();

		//read frame data, one frame at a time, or after we have displayed the second field
		perfTimer.restart();
		if ((params.interlaced_source == 0 || field == 0) && repeat == 0) {
			videoData = glw.vt->getNextFrame();
			direction = glw.vt->getDirection();
		}
		addStatPerfInt("GetFrame", perfTimer.elapsed());

		if (DEBUG)
			printf("videoData=%p\n", videoData);

		if (videoData) {

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
				glOrtho(0, videoData->Ywidth, 0, videoData->Yheight, -1, 1);

				doResize = true;
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
			updateShaderVars(programs[currentShader], videoData, params, colour_matrix);
			addStatPerfInt("UpdateVars", perfTimer.elapsed());

			//if the size of the window has changed (doResize)
			//or the shape of the video in the window has changed
			if (doResize || (params.view_valid == false)) {
				//resize the viewport, once we have some video
				//if(DEBUG) printf("Resizing to %d, %d\n", displaywidth, displayheight);

				float sourceAspect = (float)videoData->Ywidth
				    / (float)videoData->Yheight;
				float displayAspect = (float)displaywidth
				    / (float)displayheight;

				if (sourceAspect == displayAspect || !params.aspect_ratio_lock) {
					glViewport(0, 0, displaywidth, displayheight);
				}
				else {
					if (displayAspect > sourceAspect) {
						//window is wider than image should be
						int width = (int)(displayheight*sourceAspect);
						int offset = (displaywidth - width) / 2;
						glViewport(offset, 0, width, displayheight);
					}
					else {
						int height = (int)(displaywidth/sourceAspect);
						int offset = (displayheight - height) / 2;
						glViewport(0, offset, displaywidth, height);
					}
				}
				doResize = false;
				params.view_valid = true;

				glClear(GL_COLOR_BUFFER_BIT);
			}

			//	//upload the texture for the frame to the GPU. This contains both fields for interlaced
			//  renderer->uploadTextures(videoData);
			//
			//	//both fields, if interlaced
			//	for(int i=0; i<=interlacedSource; i++) {
			//
			//		//load the field and direction variables into the YUV->RGB shader
			//		int i = glGetUniformLocationARB(programs[currentShader], "field");
			//		if (direction >= 0)
			//			glUniform1iARB(i, field);
			//		else
			//			glUniform1iARB(i, 1 - field);
			//
			//		//the frame repeater renders to an intermediate framebuffer object
			//		repeater->setupRGBdestination();
			//
			//		//render the YUV texture to the framebuffer object
			//		renderer->renderVideo(videoData, programs[currentShader]);
			//
			//		//repeatedly render from the framebuffer object to the screen
			//		repeater->setupScreenDestination();
			//
			//		for(int r=0; r<frameRepeats;) {
			//
			//			//render the video from the framebuffer object to the screen
			//			repeater.renderVideo(videoData);
			//
			//			glFlush();
			//			glw.swapBuffers();
			//
			//			//the frame repeater returns the number of times it managed to repeat the frame
			//			//this may be more than once, depending on the platform
			//			r += repeater->repeat(frameRepeats);
			//		}
			//	}
			//
			//

			if (params.deinterlace) {
				glUseProgramObjectARB(programs[currentShader]);
				int i = glGetUniformLocationARB(programs[currentShader],
				                                "field");

				/* swap the field order when playing interlaced pictures backwards */
				if (direction >= 0)
					glUniform1iARB(i, field);
				else
					glUniform1iARB(i, 1 - field);

				glUseProgramObjectARB(0);
			}

			//upload the texture data for each new frame, or for before we render the first field
			//of interlaced material
			perfTimer.restart();
			if ((params.interlaced_source == 0 || field == 0) && repeat == 0)
				renderer->uploadTextures(videoData);
			addStatPerfInt("Upload", perfTimer.elapsed());

			perfTimer.restart();
			renderer->renderVideo(videoData, programs[currentShader]);
			addStatPerfInt("RenderVid", perfTimer.elapsed());

#ifdef WITH_OSD
			perfTimer.restart();
			if(osd) osd->render(videoData, params);
			addStatPerfInt("RenderOSD", perfTimer.elapsed());
#endif


		}

		glFlush();
		perfTimer.restart();
		glw.swapBuffers();
		addStatPerfInt("SwapBuffers", perfTimer.elapsed());

		//move to the next field when the first has been repeated the required number of times
		if (params.interlaced_source && (repeat == params.frame_repeats)) {
			//move to the next field
			field++;
			if (field > 1)
				field = 0;
		}

		//repeat the frame the required number of times
		if (repeat < params.frame_repeats) {
			repeat++;
			if (repeat == params.frame_repeats) {
				repeat = 0;
			}
		}

		if (repeat == 0) {
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

