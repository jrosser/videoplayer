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

#ifndef __GLVIDEO_RTADAPTOR
#define __GLVIDEO_RTADAPTOR

class GLvideo_rtAdaptor {
public:
	/* perform any initialization to allow the current thread to render
	 * into a particular drawable */
	virtual void init() = 0;

	/* swap the front and back buffers of the current drawable */
	virtual void swapBuffers() = 0;
};

class QGLWidget;
class QWidget;

/* factory to create an adaptor for reusing the QT GL rendering context
 * and passing all calls through the QT connection to the paintdevice */
GLvideo_rtAdaptor* mkGLvideo_rtAdaptorQT(QGLWidget&);

#ifdef Q_WS_X11
/* factory to create an adaptor for using an independent X11 connection
 * with its own GL rendering context.  This works around an issue when
 * using QT with X11/XCB, whereby glXSwapBuffers would block, waiting for
 * something to kick the X event loop. */
GLvideo_rtAdaptor* mkGLvideo_rtAdaptorQTX11(QWidget&);
#endif

#endif
