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

#include "GLvideo_mt.h"
#include "GLvideo_rt.h"
#include "videoTransport.h"
#include "frameQueue.h"

#include "GLvideo_params.h"

GLvideo_mt::GLvideo_mt(QWidget* parent, VideoTransport *v, FrameQueue *f,
                       GLvideo_params& vr_params) :
	QGLWidget(parent), vt(v), fq(f), renderThread(new GLvideo_rt(*this, vr_params)), vr_params(vr_params)
{
	alwaysHideMouse = false;
	setMouseTracking(true);
	connect(&mouseHideTimer, SIGNAL(timeout()), this, SLOT(hideMouse()));
	mouseHideTimer.setSingleShot(true);
}

void GLvideo_mt::stop()
{
}

void GLvideo_mt::start()
{
	renderThread->start(/*QThread::TimeCriticalPriority*/);
}

/* Reveal the mouse whenever it is moved,
 * Cause it to be hidden 1second later */
void GLvideo_mt::mouseMoveEvent(QMouseEvent *ev)
{
	ev=ev;

	if (!mouseHideTimer.isActive() && alwaysHideMouse==false)
		setCursor(QCursor(Qt::ArrowCursor));
	mouseHideTimer.start(1000);
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

void GLvideo_mt::initializeGL()
{
	//handled by the worker thread
}

/* rendering is done here. */
void GLvideo_mt::paintGL()
{
}

void GLvideo_mt::paintEvent(QPaintEvent * event)
{
	event=event;
	//absorb any paint events - let the worker thread update the window
}

void GLvideo_mt::resizeEvent(QResizeEvent * event)
{
	//added at the suggestion of Trolltech - if the rendering thread
	//is slow to start the first window resize event will issue a makeCurrent() before
	//the rendering thread does, stealing the openGL context from it
	renderThread->resizeViewport(event->size().width(), event->size().height());
}
