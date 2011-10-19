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
