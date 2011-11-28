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

#ifndef GLVIDEO_RT_H_
#define GLVIDEO_RT_H_

#include <QThread>

class GLvideo_rtAdaptor;
class VideoTransport;
namespace GLVideoRenderer
{
class GLVideoRenderer;
}
struct GLvideo_params;
class GLvideo_osd;

class GLvideo_rt : public QThread {
public:

	GLvideo_rt(GLvideo_rtAdaptor* gl, VideoTransport *vt, GLvideo_params& params);
	~GLvideo_rt();
	void resizeViewport(int w, int h);
	void run();

private:
	enum ShaderPrograms {
		Progressive = 0x0,
		Deinterlace = 0x1,
		shaderUYVY = 0,
		shaderPlanar = 2,
		/* Increment in steps of 2 */
		shaderBogus,
		shaderMax};

	void compileFragmentShaders();

	bool doRendering;

	unsigned programs[shaderMax]; //handles for the shaders and programs
	GLvideo_rtAdaptor *gl; /* widget/interface providing a GL context */

	VideoTransport *vt;

	GLvideo_osd *osd;
	GLVideoRenderer::GLVideoRenderer *renderer[2];
	GLvideo_params& params;

	int displaywidth;
	int displayheight;
};

#endif /*GLVIDEO_RT_H_*/