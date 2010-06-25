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

#ifndef GLVIDEO_MT_H
#define GLVIDEO_MT_H

#include <QtGui>
/* glew.h must be included before gl.h, which QGLWidget includes */
#include <GL/glew.h>
#include <QGLWidget>

class GLvideo_rt;
class VideoTransport;
struct GLvideo_params;

class GLvideo_mt : public QGLWidget
{
	Q_OBJECT

public:
	GLvideo_mt(QWidget *p, VideoTransport *vt, GLvideo_params& vr_params);
	~GLvideo_mt();
	void setFrameRepeats(int repeats);
	void setFontFile(QString &fontFile);

	void start();

	GLvideo_rt* renderThread;
	GLvideo_params& vr_params;

protected:

public slots:
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
