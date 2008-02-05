/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_mt.h,v 1.25 2008-02-05 00:12:06 asuraparaju Exp $
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

#ifndef GLVIDEO_MT_H
#define GLVIDEO_MT_H

#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include "glext.h"

#include <QtGui>
#include <QGLWidget>

#include "GLvideo_rt.h"
#include "videoRead.h"

class MainWindow;

class GLvideo_mt : public QGLWidget
{
    Q_OBJECT

public:
    GLvideo_mt(VideoRead &vr);
    void setFrameRepeats(int repeats);
    void setFramePolarity(int p);
    void setFontFile(QString &fontFile);
    VideoRead &vr;
    void stop();

    GLvideo_rt renderThread;

protected:

public slots:
    void toggleOSD();
    void togglePerf();
    void toggleAspectLock();
    void toggleLuminance();
    void toggleChrominance();
    void toggleDeinterlace();
    void toggleMatrixScaling();
    void setInterlacedSource(bool i);
    void setDeinterlace(bool d);
    void setLuminanceOffset1(float o);
    void setChrominanceOffset1(float o);
    void setLuminanceMultiplier(float m);
    void setChrominanceMultiplier(float m);
    void setLuminanceOffset2(float o);
    void setChrominanceOffset2(float o);
    void setMatrixScaling(bool s);
    void setMatrix(float Kr, float Kg, float Kb);
    void setCaption(QString&);
    void setOsdScale(float s);
    void setOsdState(int s);
    void setOsdTextTransparency(float t);
    void setOsdBackTransparency(float t);
    void setAlwaysHideMouse(bool h);

private slots:
    void hideMouse();

private:
    void initializeGL();

    void paintGL();
    void paintEvent(QPaintEvent * event);
    void resizeEvent(QResizeEvent * event);

    void mouseMoveEvent(QMouseEvent *ev);
    bool alwaysHideMouse;
    QTimer mouseHideTimer;
};

#endif
