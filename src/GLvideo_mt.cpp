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

#include <QtGui>

#include "GLvideo_mt.h"
#include "GLvideo_rt.h"
#include "videoTransport.h"

#include "GLvideo_rtAdaptor.h"
#ifdef Q_WS_X11
# define RTADAPTOR(obj) mkGLvideo_rtAdaptorQTX11(obj)
#else
# define RTADAPTOR(obj) mkGLvideo_rtAdaptorQT(obj)
#endif

GLvideo_mt::GLvideo_mt(QWidget* parent, VideoTransport *v, GLvideo_params& vr_params, GLfrontend_old* frontend)
: QGLWidget(parent)
, renderThread(new GLvideo_rt(RTADAPTOR(*this), v, vr_params, frontend))
, vr_params(vr_params)
, done_qt_glinit(0)
{
	alwaysHideMouse = false;
	setMouseTracking(true);
	connect(&mouseHideTimer, SIGNAL(timeout()), this, SLOT(hideMouse()));
	mouseHideTimer.setSingleShot(true);
}

GLvideo_mt::~GLvideo_mt()
{
	if(renderThread) delete renderThread;
}

/* Reveal the mouse whenever it is moved,
 * Cause it to be hidden 1second later */
void GLvideo_mt::mouseMoveEvent(QMouseEvent *ev)
{
	if (!mouseHideTimer.isActive() && alwaysHideMouse==false)
		setCursor(QCursor(Qt::ArrowCursor));
	mouseHideTimer.start(1000);

	/* allow mouse event to bubble up to next widget */
	ev->ignore();
}

void GLvideo_mt::hideMouse()
{
	setCursor(QCursor(Qt::BlankCursor));
}

void GLvideo_mt::setAlwaysHideMouse(bool h)
{
	alwaysHideMouse=h;

	if (alwaysHideMouse)
		hideMouse();
}

#include <iostream>
void GLvideo_mt::initializeGL()
{
	/* initializeGL is called by QT from QGLWidget::glInit. */
	done_qt_glinit = 1;
	/* All GL commands are handled by the rendering thread */
	doneCurrent();
	renderThread->start(/*QThread::TimeCriticalPriority*/);
}

void GLvideo_mt::paintGL()
{
	/* All GL commands are handled by the rendering thread */
}

void GLvideo_mt::resizeGL(int w, int h)
{
	/* Signal the resize to the rendering thread, but don't issue
	 * any GL commands in this context */
	renderThread->resizeViewport(w, h);
}

void GLvideo_mt::paintEvent(QPaintEvent * event)
{
	/* Painting is handled by the worker thread, however, QT internally
	 * initializes the GL context and state by QGLWidget::glInit() if
	 * uninitialized during handling paintEvent.
	 * If no initialization has occured, pass the call to the parent.
	 * However, all subsequent calls must be absorbed, since QT's
	 * default paintEvent handler will call makeCurrent() and play
	 * havock with the rendering context */
	if (!done_qt_glinit)
		QGLWidget::paintEvent(event);
}

void GLvideo_mt::resizeEvent(QResizeEvent * event)
{
	/* Painting is handled by the worker thread, however, QT internally
	 * initializes the GL context and state by QGLWidget::glInit() if
	 * uninitialized during handling resizeEvent.
	 * If no initialization has occured, pass the call to the parent.
	 * However, all subsequent calls must be absorbed, since QT's
	 * default resizeEvent handler will call makeCurrent() and play
	 * havock with the rendering context */
	if (!done_qt_glinit)
		QGLWidget::resizeEvent(event);
	else
		resizeGL(event->size().width(), event->size().height());
}
