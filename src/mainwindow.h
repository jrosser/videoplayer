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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "GLvideo_params.h"

class VideoTransportQT;

class GLvideo_mt;
class GLfrontend_old;
struct GLvideo_params;

struct Qt_params {
	bool start_fullscreen;
	bool hidemouse;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(GLvideo_params&, Qt_params&, VideoTransportQT*, GLfrontend_old*);

	QSize sizeHint() const {
		return QSize(size_hint_w, size_hint_h);
	}

protected:
	void wheelEvent(QWheelEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);

private slots:
	void toggleFullScreen();
	void escapeFullScreen();
	void setHDTVMatrix();
	void setSDTVMatrix();
	void setUserMatrix();
	void togglePerf();
	void toggleOSD();
	void toggleAspectLock();
	void toggleLuminance();
	void toggleChrominance();
	void toggleDeinterlace();
	void setUserMatrixScaling();
	void setVideoMatrixScaling();
	void setCgrMatrixScaling();
	void setZoomIn();
	void setZoomOut();
	void setZoomReset();
	void hideMouse();

private:
	void createActions(void);

	void setFullScreen(bool);

	GLvideo_mt *glvideo_mt;

	GLvideo_params& vr_params;
	GLvideo_params vr_params_orig;

	VideoTransportQT *video_transport;

	GLfrontend_old *gl_frontend;

	unsigned size_hint_w; /* ideal window width */
	unsigned size_hint_h; /* ideal window height */

	int mouse_drag_start_x;
	int mouse_drag_start_y;

	QTimer mouse_hide_timer;
};

#endif
