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

#include <QtGui>
#include "GLvideo_rt.h"
#include "GLvideo_mt.h"
#include "GLvideo_rt.h"
#include "GLvideo_tradtex.h"
#include "GLvideo_pbotex.h"

#ifdef Q_WS_X11
#include "GLvideo_x11rep.h"
#endif

#ifdef WITH_OSD
#include "FTGL/FTGLPolygonFont.h"
#endif

#include "videoData.h"
#include "videoTransport.h"
#include "frameQueue.h"

#include "util.h"

#include "GLvideo_params.h"

#include "shaders.h"

#ifdef Q_OS_MACX
#include "agl_getproc.h"
#endif

//FIXME - nasty global variables
//rendering performance monitoring
int perf_getParams;
int perf_readData;
int perf_convertFormat;
int perf_updateVars;
int perf_repeatWait;
int perf_upload;
int perf_renderVideo;
int perf_renderOSD;
int perf_swapBuffers;
int perf_interval;

//I/O performance
int perf_futureQueue;
int perf_pastQueue;
int perf_IOLoad;
int perf_IOBandwidth;

#define DEBUG 0

GLvideo_rt::GLvideo_rt(GLvideo_mt &gl, GLvideo_params& params) :
	QThread(), glw(gl), params(params)
{
	renderer = new GLVideoRenderer::TradTex();
	//renderer = new GLVideoRenderer::PboTex();
}

void GLvideo_rt::stop()
{
	doRendering = false;
	wait();
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

#ifdef WITH_OSD
void GLvideo_rt::renderOSD(VideoData *videoData, FTFont *font, float fps,
                           int osd, float osdScale)
{
	//bounding box of text
	float cx1, cy1, cz1, cx2, cy2, cz2;

	//text string
	char str[255];
	switch (osd) {
	case OSD_FRAMENUM:
		sprintf(str, "%06ld", videoData->frameNum);
		font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_FPS:
		sprintf(str, "%06ld, fps:%3.2f", videoData->frameNum, fps);
		font->BBox("000000, fps:000.00", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_INT:
		sprintf(str, "%06ld, %s:%s", videoData->frameNum,
		        params.interlaced_source ? "Int" : "Prog",
		        params.deinterlace ? "On" : "Off");
		font->BBox("000000, Prog:Off", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_CAPTION:
		sprintf(str, "%s", params.caption.toLatin1().constData());
		font->BBox(str, cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	default:
		str[0] = '\0';
	}

	//text box location, defaults to bottom left
	float tx=0.05 * videoData->Ywidth;
	float ty=0.05 * videoData->Yheight;

	if (osd==4) {
		//put the caption in the middle of the screen
		tx = videoData->Ywidth - ((cx2 - cx1) * osdScale);
		tx/=2;
	}

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	bx1 = cx1 - border;
	by1 = cy1 - border;
	bx2 = cx2 + border;
	by2 = cy2 + border;

	//box beind text
	glPushMatrix();
	glTranslated(tx, ty, 0);
	glScalef(osdScale, osdScale, 0); //scale the on screen display

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, params.osd_back_alpha);

	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();

	//text
	glColor4f(1.0, 1.0, 1.0, params.osd_text_alpha);
	glEnable(GL_POLYGON_SMOOTH);
	font->Render(str);
	glDisable(GL_POLYGON_SMOOTH);
	glPopMatrix();
}

void drawText(char *str, FTFont *font)
{
	glEnable(GL_POLYGON_SMOOTH);
	glPushMatrix();
	font->Render(str);
	glPopMatrix();
	glDisable(GL_POLYGON_SMOOTH);
}

void draw2Text(const char *str1, const char *str2, float h_spacing,
               FTFont *font)
{
	glEnable(GL_POLYGON_SMOOTH);
	glPushMatrix();

	glPushMatrix();
	font->Render(str1);
	glPopMatrix();

	glTranslatef(h_spacing, 0, 0);

	glPushMatrix();
	font->Render(str2);
	glPopMatrix();

	glPopMatrix();
	glDisable(GL_POLYGON_SMOOTH);
}

void drawPerfTimer(const char *str, int num, const char *units,
                   float h_spacing, FTFont *font)
{
	char str2[255];
	sprintf(str2, "%d%s", num, units);
	draw2Text(str, str2, h_spacing, font);
}

void GLvideo_rt::renderPerf(VideoData *videoData, FTFont *font)
{
	float tx = 0.05 * videoData->Ywidth;
	float ty = 0.95 * videoData->Yheight;
	char str[255];
	float cx1, cy1, cz1, cx2, cy2, cz2;

	font->BBox("0", cx1, cy1, cz1, cx2, cy2, cz2);
	float spacing = (cy2-cy1) * 1.5;
	float width = cx2 - cx1;

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	bx1 = -border;
	by1 = spacing;
	bx2 = width * 25;
	by2 = (spacing * -15) - border*2;

	glPushMatrix();
	glTranslated(tx, ty, 0); //near the top left corner
	glScalef(0.25, 0.25, 0); //reduced in size compared to OSD

	//box beind text
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, 0.7);

	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();

	glColor4f(0.0, 1.0, 0.0, 0.5);

	sprintf(str, "Rendering");
	drawText(str, font);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("ReadData", perf_readData, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("ConvertFormat", perf_convertFormat, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("UpdateVars", perf_updateVars, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RepeatWait", perf_repeatWait, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Upload", perf_upload, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RenderVideo", perf_renderVideo, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RenderOSD", perf_renderOSD, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("SwapBuffers", perf_swapBuffers, "ms", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Interval", perf_interval, "ms", width*15, font);

	glColor4f(0.0, 1.0, 0.0, 0.5);

	glTranslated(0, -spacing, 0);
	glTranslated(0, -spacing, 0);
	sprintf(str, "I/O");
	drawText(str, font);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Future Queue", perf_futureQueue, "", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Past Queue", perf_pastQueue, "", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("I/O Thread Load", perf_IOLoad, "%", width*15, font);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("ReadRate", perf_IOBandwidth, "MB/s", (int)width*17, font);

	glPopMatrix();
}
#endif

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
	float fps = 0;

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

#ifdef WITH_OSD
	FTFont *font = NULL;
#endif

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
	glEnable(GL_BLEND);
	glClear(GL_COLOR_BUFFER_BIT);

	compileFragmentShaders();

	while (doRendering) {

		bool createGLTextures = false;

		perfTimer.start();

		//read frame data, one frame at a time, or after we have displayed the second field
		perfTimer.restart();
		if ((params.interlaced_source == 0 || field == 0) && repeat == 0) {
			videoData = glw.vt->getNextFrame();
			direction = glw.vt->getDirection();
			perf_futureQueue = glw.fq->getFutureQueueLen();
			perf_pastQueue = glw.fq->getPastQueueLen();
			//perf_IOLoad      = glw.fq->getIOLoad();
			//perf_IOBandwidth = glw.fq->getIOBandwidth();
			perf_readData = perfTimer.elapsed();
		}

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
			perf_convertFormat = perfTimer.elapsed();

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
			}

			//update the uniform variables in the fragment shader
			perfTimer.restart();
			updateShaderVars(programs[currentShader], videoData, params, colour_matrix);
			perf_updateVars = perfTimer.elapsed();

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
			}

#ifdef WITH_OSD
			if (!params.osd_valid) {
				if (DEBUG)
					printf("Changing font\n");
				if (font)
					delete font;

				font = new FTGLPolygonFont(params.font_file.toLatin1().constData());
				if (!font->Error()) {
					font->FaceSize(72);
					font->CharMap(ft_encoding_unicode);
				}
				else {
					delete font;
					font = NULL;
				}
			}
#endif

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

			/* reset all state vars */
			params.matrix_valid = true;
			params.osd_valid = true;
			params.view_valid = true;
			doResize = false;

			//upload the texture data for each new frame, or for before we render the first field
			//of interlaced material
			perfTimer.restart();
			if ((params.interlaced_source == 0 || field == 0) && repeat == 0)
				renderer->uploadTextures(videoData);
			perf_upload = perfTimer.elapsed();

			perfTimer.restart();
			renderer->renderVideo(videoData, programs[currentShader]);
			perf_renderVideo = perfTimer.elapsed();

#ifdef WITH_OSD
			perfTimer.restart();
			if ((params.osd_bot) && font != NULL)
				renderOSD(videoData, font, fps, params.osd_bot,
				          params.osd_scale);
			perf_renderOSD = perfTimer.elapsed();
			if (params.osd_perf && font != NULL)
				renderPerf(videoData, font);
#endif
		}

		glFlush();
		perfTimer.restart();
		glw.swapBuffers();
		perf_swapBuffers = perfTimer.elapsed();

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
			perf_interval = frameIntervalTime.elapsed();
			//if(DEBUG) printf("interval=%d\n", perf_interval);
			frameIntervalTime.restart();

			//calculate FPS
			if (fpsAvgPeriod == 0) {
				int fintvl = fpsIntervalTime.elapsed();
				if (fintvl)
					fps = 10*1e3/fintvl;
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

