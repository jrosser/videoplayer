/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2011 BBC Research and Development
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

#include <QGLWidget>
#ifdef Q_WS_X11
# include <QWidget>
# include <QX11Info>
# include <X11/Xlib.h>
# include <GL/glx.h>
#endif

#include "GLvideo_rtAdaptor.h"

class GLvideo_rtAdaptorQT : public GLvideo_rtAdaptor {
public:
	GLvideo_rtAdaptorQT(QGLWidget& glw)
		: glw(glw)
	{}

	/* make the window's openGL context current for the calling
	 * thread.  It is important that prior to this call, that the
	 * context is released from any other thread via glw.doneCurrent()
	 */
	void init() {
		glw.makeCurrent();
	}

	void swapBuffers() {
		glw.swapBuffers();
	}

private:
	QGLWidget& glw;
};

GLvideo_rtAdaptor* mkGLvideo_rtAdaptorQT(QGLWidget& qtwidget)
{
	return new GLvideo_rtAdaptorQT(qtwidget);
}

#ifdef Q_WS_X11
class GLvideo_rtAdaptorQTX11 : public GLvideo_rtAdaptor {
public:
	/*
	 * Lame hack to work around issues with QT, X11, XCB and GLX stalling
	 * when threads are enabled.
	 * Obtain a new GL context and run with a completely seperate set of
	 * X communication, so that QT can't break it.
	 */
	GLvideo_rtAdaptorQTX11(QWidget& qtwidget) {
		win = qtwidget.winId();
		const QX11Info& x11info = qtwidget.x11Info();
		screen = x11info.screen();
		visualid = static_cast<Visual*>(x11info.visual())->visualid;
	}

	void init();
	void swapBuffers();

private:
	Window win;
	int screen;
	VisualID visualid;
	Display *dpy;
	GLXContext context;
};

void GLvideo_rtAdaptorQTX11::init()
{
	dpy = XOpenDisplay(DisplayString(QX11Info::display()));

	/* Find the Visual */
	XVisualInfo *visual;
	XVisualInfo visual_template;
	visual_template.visualid = visualid;
	visual_template.screen = screen;
	int visual_num;
	visual = XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &visual_template, &visual_num);

	/* Create Context & install in thread */
	context = glXCreateContext(dpy, visual, NULL, True);
	glXMakeCurrent(dpy, win, context);

	XFree(visual);
};

void GLvideo_rtAdaptorQTX11::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

GLvideo_rtAdaptor* mkGLvideo_rtAdaptorQTX11(QWidget& qtwidget)
{
	return new GLvideo_rtAdaptorQTX11(qtwidget);
}
#endif
