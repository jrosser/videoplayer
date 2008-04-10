/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: GLvideo_rt.h,v 1.33 2008/03/13 11:38:49 jrosser Exp $
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

#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#include <GL/glew.h>

#ifdef Q_OS_WIN32
#include <GL/wglew.h>
#endif

#ifdef Q_WS_X11
#include <GL/glxew.h>
#endif

#include <QtGui>

#include "GLvideo_renderer.h"

class GLvideo_mt;
class FTFont;
class VideoData;
namespace GLVideoRenderer { class GLVideoRenderer; }
class GLvideo_params;

class GLvideo_rt : public QThread
{
public:

    GLvideo_rt(GLvideo_mt &glWidget, GLvideo_params& params);
	void resizeViewport(int w, int h);
	void run();

private:
    void compileFragmentShaders();
    void compileFragmentShader(int n, const char *src);
#ifdef HAVE_FTGL
    void renderOSD(VideoData *videoData, FTFont *font, float fps, int osd, float osdScale);
    void renderPerf(VideoData *videoData, FTFont *font);
#endif

	enum ShaderPrograms { shaderUYVY, shaderPlanar, shaderMax };

    GLuint programs[shaderMax];    //handles for the shaders and programs
    GLuint shaders[shaderMax];
    GLint compiled[shaderMax];
    GLint linked[shaderMax];    //flags for success
    GLvideo_mt &glw;            //parent widget

    QMutex mutex;

    GLVideoRenderer::GLVideoRenderer *renderer;
	GLvideo_params& params;

    bool doResize;
    int displaywidth;
    int displayheight;
};


#endif /*GLVIDEO_RT_H_*/
