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

#ifndef GLVIDEO_OSD_H_
#define GLVIDEO_OSD_H_

#include <GL/glew.h>

#include "GLvideo_renderer.h"
#include <FTGL/ftgl.h>

class FTFont;
class VideoData;
class GLvideo_params;

class GLvideo_osd {
public:

	void render(VideoData *videoData, GLvideo_params &params);
	~GLvideo_osd();
	
private:

	void renderOSD(VideoData *videoData, GLvideo_params &params);
	void drawText(const char *str);
	void draw2Text(const char *str1, const char *str2, float h_spacing);
	void drawPerfTimer(const char *str, int num, const char *units, float h_spacing);
	void renderStats(VideoData *);
	FTFont *font;
};

#endif /*GLVIDEO_OSD_H_*/
