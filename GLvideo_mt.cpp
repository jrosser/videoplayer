/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_mt.cpp,v 1.29 2008-03-13 11:38:49 jrosser Exp $
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

GLvideo_mt::GLvideo_mt(QWidget* parent, VideoTransport *v, FrameQueue *f)
    : QGLWidget(parent), vt(v), fq(f), renderThread(new GLvideo_rt(*this))
{
    setMouseTracking(true);
    connect(&mouseHideTimer, SIGNAL(timeout()), this, SLOT(hideMouse()));
    mouseHideTimer.setSingleShot(true);

    /* NB, a) this helps to be last
     *     b) don't put HideMouse anywhere near here */
    renderThread->start(QThread::TimeCriticalPriority);
}

void GLvideo_mt::stop()
{
    renderThread->stop();
    renderThread->wait();
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

    if(alwaysHideMouse) hideMouse();
}


void GLvideo_mt::setOsdScale(float s)
{
    renderThread->setOsdScale(s);
}

void GLvideo_mt::setOsdState(int s)
{
    renderThread->setOsdState(s);
}

void GLvideo_mt::setOsdTextTransparency(float t)
{
    renderThread->setOsdTextTransparency(t);
}

void GLvideo_mt::setOsdBackTransparency(float t)
{
    renderThread->setOsdBackTransparency(t);
}

void GLvideo_mt::togglePerf()
{
    renderThread->togglePerf();
}

void GLvideo_mt::toggleOSD()
{
    renderThread->toggleOSD();
}

void GLvideo_mt::toggleAspectLock()
{
    renderThread->toggleAspectLock();
}

void GLvideo_mt::toggleLuminance()
{
    renderThread->toggleLuminance();
}

void GLvideo_mt::toggleChrominance()
{
    renderThread->toggleChrominance();
}

void GLvideo_mt::setInterlacedSource(bool i)
{
    renderThread->setInterlacedSource(i);
}

void GLvideo_mt::toggleDeinterlace()
{
    renderThread->toggleDeinterlace();
}

void GLvideo_mt::setDeinterlace(bool d)
{
    renderThread->setDeinterlace(d);
}

void GLvideo_mt::setMatrixScaling(bool s)
{
    renderThread->setMatrixScaling(s);
}

void GLvideo_mt::setMatrix(float Kr, float Kg, float Kb)
{
    renderThread->setMatrix(Kr, Kg, Kb);
}

void GLvideo_mt::toggleMatrixScaling()
{
    renderThread->toggleMatrixScaling();
}

void GLvideo_mt::initializeGL()
{
    //handled by the worker thread
}

void GLvideo_mt::paintGL()
{
    //handled by the worker thread
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

void GLvideo_mt::setFrameRepeats(int repeats)
{
    renderThread->setFrameRepeats(repeats);
}

void GLvideo_mt::setFontFile(QString &fontFile)
{
    renderThread->setFontFile(fontFile.toLatin1().data());
}

void GLvideo_mt::setLuminanceMultiplier(float m)
{
    renderThread->setLuminanceMultiplier(m);
}

void GLvideo_mt::setChrominanceMultiplier(float m)
{
    renderThread->setChrominanceMultiplier(m);
}

void GLvideo_mt::setLuminanceOffset1(float o)
{
    renderThread->setLuminanceOffset1(o);
}

void GLvideo_mt::setChrominanceOffset1(float o)
{
    renderThread->setChrominanceOffset1(o);
}

void GLvideo_mt::setLuminanceOffset2(float o)
{
    renderThread->setLuminanceOffset2(o);
}

void GLvideo_mt::setChrominanceOffset2(float o)
{
    renderThread->setChrominanceOffset2(o);
}

void GLvideo_mt::setCaption(QString &caption)
{
    renderThread->setCaption(caption.toLatin1().data());
}
