/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2008-2010 BBC Research
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

#ifndef GLFRONTEND_OLD_H
#define GLFRONTEND_OLD_H

class VideoTransport;
struct GLvideo_params;

class GLfrontend_old
{
public:
	GLfrontend_old(GLvideo_params&, VideoTransport *);
	~GLfrontend_old();

	void render();
	void resizeViewport(int w, int h);
	void setLayoutGrid(int h, int v);
	void getOptimalDimensions(unsigned& w, unsigned& h);

	/** set the internal border width of the layout grid to @b@ px */
	void setLayoutGridBorder(int b) { grid_border = b; }

private:
	void init();
	bool init_done;

	VideoTransport *vt;

	GLvideo_params& params;
	bool doResize;
	int displaywidth;
	int displayheight;

	int layout_grid_h;
	int layout_grid_v;

	int grid_border; /* width of layout grid internal border in pixels */

	struct Layout {
		int xoff; /* window coordinate of viewport origin */
		int yoff; /* window coordinate of viewport origin */
		int width; /* width of viewport area */
		int height; /* height of viewport area */
		int source; /* video transport queue number */
	} *layouts;

	/* Y'CbCr->RGB matrix */
	float colour_matrix[4][4];

	/* shader programs */
	unsigned int programs[4];
};

#endif
