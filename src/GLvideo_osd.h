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

#include "freetype-gl/freetype-gl.h"
#include "freetype-gl/vertex-buffer.h"

typedef struct { float r; float g; float b;  float a; } rgba_t;

typedef struct markup {
	texture_font_t* font;
	rgba_t foreground_colour;
} markup_t;

struct VideoData;
struct GLvideo_params;

class GLvideo_osd {
public:
	GLvideo_osd(GLvideo_params& );

	void render(unsigned viewport_width, unsigned viewport_height, VideoData *videoData, GLvideo_params &params);
	~GLvideo_osd();
	
private:

	void renderOSD(unsigned viewport_width, unsigned viewport_height, VideoData *videoData, GLvideo_params &params);
	void renderStats(unsigned viewport_width, unsigned viewport_height, GLvideo_params &);

	texture_atlas_t* font_atlas;
	vertex_buffer_t* vertex_buffer;
	markup_t markup_normal;
	markup_t markup_stats;

	float text_val_start_pos;
};

#endif /*GLVIDEO_OSD_H_*/
