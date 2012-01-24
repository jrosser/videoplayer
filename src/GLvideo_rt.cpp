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
# include "osx.h"
#endif
#include "GLvideo_rtAdaptor.h"

#include "GLvideo_rt.h"
#include "GLvideo_osd.h"
#include "GLfrontend_old.h"

#include "videoData.h"
#include "videoTransport.h"
#include "readerInterface.h"
#include "stats.h"

#include "GLvideo_params.h"

#define DEBUG 0


GLvideo_rt::GLvideo_rt(GLvideo_rtAdaptor *gl, VideoTransport *vt, GLvideo_params& params, GLfrontend_old* frontend)
	: QThread()
	, gl(gl)
	, vt(vt)
	, frontend(frontend)
	, params(params)
{
#ifdef WITH_OSD
	osd = new GLvideo_osd(params);
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
	Stats::Section& stats = Stats::getSection("OpenGL");

	if (DEBUG)
		printf("Starting renderthread\n");

	/* initialize the gl windowing adaptor */
	gl->init();

	frontend->resizeViewport(viewport_width, viewport_height);

	//monitoring
	QTime fps_timer;
	QTime perfTimer; //performance timer for measuring indivdual processes during rendering
	int fps_avg_period_actual = 1; /* used to caluclate actual display fps */
	int fps_avg_period_effective = 1; /* used to calculate video fps */
	int fps_avg_period_effective_start = 0;
	int fps_avg_period_actual_start = 0;
	int fps_period_start = 0;

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
	osxSwapInterval(1);
#endif

	/* startup: clear the buffers to stop rubbish being presented
	 * before first video frame is presented */
	glClearColor(0, 0, 0, 0);
	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);

	fps_timer.start();

	while (doRendering) {

		perfTimer.start();
		/* request the next frame.  videoData might not change if using repeats
		 * or displaying fields */
		/* advance the video transport to the next frame.
		 * frames will be uploaded by GLactions called by the frontend */
		bool is_new_frame_period = vt->advance();
		VideoData* videoData = vt->getFrame(0);
		ReaderInterface* videoData_io = vt->getReader(0);

		perfTimer.restart();
		frontend->render();
		addStat(stats, "Frontend", perfTimer.elapsed(), "ms");

#ifdef WITH_OSD
		perfTimer.restart();

		glViewport(0, 0, viewport_width, viewport_height);
		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, viewport_width, 0, viewport_height, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		if (videoData_io && videoData) {
			const QString& tmp = videoData_io->getCaption(videoData->frame_number);
			params.caption = tmp;
		}
		if(osd && videoData) osd->render(viewport_width, viewport_height, videoData, params);

		addStat(stats, "OSD", perfTimer.elapsed(), "ms");
#endif

		perfTimer.restart();
		gl->swapBuffers();
		addStat(stats, "SwapBuffers", perfTimer.elapsed(), "ms");

		int fps_timer_time = fps_timer.elapsed();
		if (is_new_frame_period && !(fps_avg_period_effective++ % 10)) {
			addStat(stats, "VideoRate", 10e3/(fps_timer_time - fps_avg_period_effective_start), "Hz");
			fps_avg_period_effective_start = fps_timer_time;
		}
		if (!(fps_avg_period_actual++ % 10)) {
			addStat(stats, "VDURate", 10e3/(fps_timer_time - fps_avg_period_actual_start), "Hz");
			fps_avg_period_actual_start = fps_timer_time;
		}
		//measure overall frame period
		addStat(stats, "Interval", fps_timer_time - fps_period_start, "ms");
		fps_period_start = fps_timer_time;

		GLenum error;
		const GLubyte* errStr;
		if ((error = glGetError()) != GL_NO_ERROR) {
			errStr = gluErrorString(error);
			fprintf(stderr, "OpenGL Error: %s\n", errStr);
		}
	}
}

