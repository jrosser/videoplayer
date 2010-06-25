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

#ifdef WITH_CONFIG_H
# include <config.h>
#endif

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
#include "GLvideo_osd.h"
#include "GLfrontend_old.h"

#include "videoData.h"
#include "videoTransport.h"
#include "stats.h"

#include "GLvideo_params.h"

#define addStatPerfFloat(name, val) addStat("OpenGL", name, val)
#define addStatPerfInt(name, val) addStatUnit("OpenGL", name, val, "ms")

#define DEBUG 0


GLvideo_rt::GLvideo_rt(GLvideo_rtAdaptor *gl, VideoTransport *vt, GLvideo_params& params) :
	QThread(), gl(gl), vt(vt), frontend(0), params(params)
{
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
#ifdef WITH_OSD
	if(osd) delete osd;
#endif
}

void GLvideo_rt::resizeViewport(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
	/* pass call on to the frontend (which manages the viewport) */
	if (frontend) {
		frontend->resizeViewport(width, height);
	}
}

void GLvideo_rt::run()
{
	if (DEBUG)
		printf("Starting renderthread\n");

	/* initialize the gl windowing adaptor */
	gl->init();

	/* create a frontend */
	/* todo: this should happen outside of this class */
	frontend = new GLfrontend_old(params, vt);
	frontend->resizeViewport(viewport_width, viewport_height);

	//monitoring
	QTime fpsIntervalTime; //measures FPS, averaged over several frames
	QTime frameIntervalTime; //measured the total period of each frame
	QTime perfTimer; //performance timer for measuring indivdual processes during rendering
	int fpsAvgPeriod=1;

	doRendering = true;

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

	while (doRendering) {

		perfTimer.start();
		/* request the next frame.  videoData might not change if using repeats
		 * or displaying fields */
		/* advance the video transport to the next frame.
		 * frames will be uploaded by GLactions called by the frontend */
		bool is_new_frame_period = vt->advance();
		VideoData* videoData = vt->getFrame();
		addStatPerfInt("GetFrame", perfTimer.elapsed());

		perfTimer.restart();
		frontend->render();
		addStatPerfInt("Frontend", perfTimer.elapsed());

#ifdef WITH_OSD
		perfTimer.restart();
		if(osd && videoData) osd->render(videoData, params);
		addStatPerfInt("RenderOSD", perfTimer.elapsed());
#endif

		perfTimer.restart();
		gl->swapBuffers();
		addStatPerfInt("SwapBuffers", perfTimer.elapsed());

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

