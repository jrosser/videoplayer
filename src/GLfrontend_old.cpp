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

#include <QTime>
#include <stdio.h>
#include <math.h>

#include <algorithm>

#include <GL/glew.h>

#include "GLfrontend_old.h"
#include "GLvideo_params.h"
#include "GLutil.h"
#include "shaders.h"

#include "colourMatrix.h"

#include "videoTransport.h"
#include "videoData.h"

#include "stats.h"

#define DEBUG 0

static int chooseShader(VideoData *video_data, GLvideo_params& params);
static void updateShaderVars(int program, VideoData *video_data, float colour_matrix[4][4]);

using namespace GLvideo;
using namespace std;

enum ShaderPrograms {
	Progressive = 0x0,
	Deinterlace = 0x1,
	shaderUYVY = 0,
	shaderPlanar = 2,
	/* Increment in steps of 2 */
	shaderBogus,
	shaderMax
};

GLfrontend_old::GLfrontend_old(GLvideo_params& params, VideoTransport* vt)
	: init_done(0)
	, vt(vt)
	, params(params)
	, doResize(0)
	, layouts(0)
	, stats(Stats::getInstance().newSection("OpenGL", this))
{
	setLayoutGrid(1, 1);
}

GLfrontend_old::~GLfrontend_old() {
}

void GLfrontend_old::setLayoutGrid(int h, int v)
{
	delete[] layouts;

	layout_grid_h = h;
	layout_grid_v = v;

	layouts = new Layout[h * v];

	for (int y = 0; y < layout_grid_v; y++) {
		for (int x = 0; x < layout_grid_h; x++) {
			Layout& l = layouts[layout_grid_h * y + x];
			l.source = layout_grid_h * y + x;
		}
	}
}

void GLfrontend_old::getOptimalDimensions(unsigned& w, unsigned& h)
{
	int num_layouts = layout_grid_h * layout_grid_v;
	VideoData *frames[16];

	/* xxx: this causes a frame to be dropped at start of file */
	if (!vt->getFrame(0))
		while (!vt->advance());

	/* load the next frame, spinning until it is available.
	 * handling the case when nothing is attached to the framequeue */
	for (int i = 0; i < num_layouts; i++) {
		frames[i] = NULL;
		try {
			frames[i] = vt->getFrame(layouts[i].source);
		} catch (...) {
			/* nothing attached to ith source */
		}
	}

	/* work out the maximum width of each column and sum */
	unsigned width = 0;
	for (int x = 0; x < layout_grid_h; x++) {
		unsigned col_width_max = 0;
		for (int y = 0; y < layout_grid_v; y++) {
			VideoData *vd = frames[layout_grid_h * y + x];
			if (!vd)
				continue;
			col_width_max = max(col_width_max, vd->data.plane[0].width);
		}
		width += col_width_max;
	}

	/* work out the maximum height of each column and sum */
	unsigned height = 0;
	for (int y = 0; y < layout_grid_v; y++) {
		unsigned row_height_max = 0;
		for (int x = 0; x < layout_grid_h; x++) {
			VideoData *vd = frames[layout_grid_h * y + x];
			if (!vd)
				continue;
			row_height_max = max(row_height_max, vd->data.plane[0].height);
		}
		height += row_height_max;
	}

	w = width;
	h = height;
}

void GLfrontend_old::init() {
	programs[shaderPlanar | Progressive] = compileFragmentShader(shaderPlanarProSrc);
	programs[shaderPlanar | Deinterlace] = compileFragmentShader(shaderPlanarDeintSrc);
	programs[shaderUYVY | Progressive] = compileFragmentShader(shaderUYVYProSrc);
	programs[shaderUYVY | Deinterlace] = compileFragmentShader(shaderUYVYDeintSrc);
}

void GLfrontend_old::resizeViewport(int width, int height)
{
	displaywidth = width;
	displayheight = height;
	doResize = true;
}

/* structure to inform on the best way to upload a particular
 * data representation. */
struct UploadSpec {
	/* the combination of src_type and src_grouping would effectively
	 * be "src_storage".  src_type also acts as a hint to GL if dst_storage
	 * was an unsized value. */
	GLuint src_type;     /* gl:type */
	GLuint src_grouping; /* gl:format */
	GLuint dst_storage;  /* gl:internalformat */
	unsigned notional_width_div; /* amount to divide plane's notional width by
	                                to obtain the texture width */
} upload_spec[] = {
	/*   P8 */ { GL_UNSIGNED_BYTE,  GL_LUMINANCE, GL_LUMINANCE8, 1 },
	/*  P16 */ { GL_UNSIGNED_SHORT, GL_LUMINANCE, GL_LUMINANCE16, 1 },
	/* UYVY */ { GL_UNSIGNED_BYTE,  GL_RGBA,      GL_RGBA8, 2 },
	/* V216 */ { GL_UNSIGNED_SHORT, GL_RGBA,      GL_RGBA16, 2 },
	/* V210 */ { GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGBA, GL_RGB10_A2, 2 },
};

GLuint
alloc_texture_new(UploadSpec *upload, unsigned width, unsigned height)
{
	GLuint texture;
	glGenTextures(1, &texture);

	/* The explicit bind to the zero pixel unpack buffer object allows
	 * passing NULL in glTexImage2d() to be unspecified texture data
	 * (ie, create the storage for the texture but don't upload antything
	 */
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	/* create the texture */
	/* this should be part of alloc texture */
	/* xxx: technically, src_grouping doesn't matter here */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, upload->dst_storage,
	             width, height, 0,
	             upload->src_grouping, upload->src_type, NULL);

	return texture;
}

GLuint
alloc_texture(UploadSpec *upload, unsigned width, unsigned height)
{
	GLuint texture;
	texture = alloc_texture_new(upload, width, height);
	return texture;
}

void unref_texture(GLuint texture)
{
	if (!glIsTexture(texture))
		return;
	glDeleteTextures(1, &texture);
}

/* upload a single texture */
PictureData<GLuint>::Plane<GLuint>
upload(PackingFmt fmt, PictureData<DataPtr>::Plane<DataPtr>& src)
{
	UploadSpec* upload = &upload_spec[fmt];
	/* in the case of packed formats, eg UYVY, the upload is done as
	 * RGBA-tuples.  The notional width is the expected width of the
	 * luma.  however, the texture width is half as wide (two luma
	 * values per tuple). */
	unsigned width = src.width / upload->notional_width_div;
	unsigned height = src.height;
	void* data = src.data->ptr;

	GLuint texture = alloc_texture(upload, width, height);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	/* upload data to the texture */
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0,
	                width, height,
	                upload->src_grouping, upload->src_type, data);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	PictureData<GLuint>::Plane<GLuint> plane = {
		std::tr1::shared_ptr<GLuint>(new GLuint(texture)),
		width,
		height,
		0,
	};

	return plane;
}

void
upload(VideoData* vd)
{
	if (!vd->gl_data.plane.empty()) {
		/* data has already been uploaded and is still avaliable */
		return;
	}

	QTime perfTimer;
	perfTimer.start();
	//perform any format conversions
	if (vd->data.packing_format == V210) {
		/* convert v210 to uyvy */
		/* todo: refactor this into a shader */
		convertV210toUYVY(vd->data);
	}
	else
	if (vd->data.packing_format == V216) {
		/* convert 16bit data to 8bit */
		/* todo: refactor into a shader */
		/* this avoids endian-ness issues between the host and the GPU
		 * and reduces the bandwidth to the GPU */
		convertV216toUYVY(vd->data);
	}
	else
	if (vd->data.packing_format == V16P) {
		convertPlanar16toPlanar8(vd->data);
	}

	perfTimer.restart();
	vd->gl_data.packing_format = vd->data.packing_format;
	vd->gl_data.chroma_format = vd->data.chroma_format;
	vd->gl_data.plane.resize(vd->data.plane.size());
	for (unsigned i = 0; i < vd->data.plane.size(); i++) {
		vd->gl_data.plane[i] = upload(vd->data.packing_format, vd->data.plane[i]);
	}
}

void unref_texture(VideoData* vd)
{
	for (unsigned i = 0; i < vd->gl_data.plane.size(); i++) {
		unref_texture(*vd->gl_data.plane[i].data);
	}
	vd->gl_data.plane.clear();
}

void render(VideoData *vd, GLuint shader_prog)
{
	glUseProgramObjectARB(shader_prog);

	for (unsigned i = 0; i < vd->gl_data.plane.size(); i++) {
		static const char* texture_names[] = {"Ytex", "Utex", "Vtex"};
		/* bind texture_name of shader_prog, to texture unit idx
		 * which can access texture[idx] */
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *vd->gl_data.plane[i].data);
		GLuint loc = glGetUniformLocationARB(shader_prog, texture_names[i]);
		glUniform1iARB(loc, i);
	}

	/* NB, texture coordinates are relative to the texture data, (0,0)
	 * is the first texture pixel uploaded, since we use two different
	 * coordinate systems, the texture appears to be upside down */
	/* xxx, move above description to texgen */
	/* xxx, why isn't this the case? */
#if 0
	static const float s_plane[] = {1, 0, 0, 0};
	static const float t_plane[] = {0, 1, 0, 0};
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, s_plane);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, t_plane);
	glMatrixMode(GL_TEXTURE);
		/* xxx:
		 *  either need to scale the texture access by notional_width_div
		 *  or, need to scale the vertex coords. */
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
#endif

	unsigned tex_width = vd->gl_data.plane[0].width;
	unsigned tex_height = vd->gl_data.plane[0].height;
	unsigned width = vd->data.plane[0].width;
	unsigned height = vd->data.plane[0].height;

	/* NB, colours below are as a diagnostic to show when texturing has failed */
	glBegin(GL_QUADS);
		glTexCoord2i(0, tex_height);
		glColor3f(1., 0., 0.);
		glVertex2i(0, 0);

		glTexCoord2i(tex_width, tex_height);
		glColor3f(0., 1., 0.);
		glVertex2i(width, 0);

		glTexCoord2i(tex_width, 0);
		glColor3f(0., 0., 1.);
		glVertex2i(width, height);

		glTexCoord2i(0, 0);
		glColor3f(0., 0., 0.);
		glVertex2i(0, height);
	glEnd();
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glUseProgramObjectARB(0);

	for (unsigned i = 0; i < vd->gl_data.plane.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	glActiveTexture(GL_TEXTURE0);
}

void GLfrontend_old::render()
{
	if (!init_done) {
		/* this must be called in a thread with the GL context */
		init();
		init_done = true;
	}

	QTime perfTimer; //performance timer for measuring indivdual processes during rendering
	perfTimer.start();

	if (params.matrix_valid == false) {
		buildColourMatrix(colour_matrix, params);
		params.matrix_valid = true;
	}

	int num_layouts = layout_grid_h * layout_grid_v;
	VideoData* frames[16];

	perfTimer.restart();
	for (int i = 0; i < num_layouts; i++) {
		try {
			frames[i] = vt->getFrame(i);
			upload(frames[i]);
		} catch (...) {
			frames[i] = NULL;
		}
	}
	addStat(*stats, "Upload", perfTimer.elapsed(), "ms");

	glPushMatrix();
	for (int x = 0; x < layout_grid_h; x++) {
		for (int y = 0; y < layout_grid_v; y++) {
			VideoData *video_data = frames[layout_grid_h * y + x];

			int vp_x_left = x * displaywidth / layout_grid_h;
			int vp_x_right = (x+1) * displaywidth / layout_grid_h;
			int vp_y_bot = y * displayheight / layout_grid_v;
			int vp_y_top = (y+1) * displayheight / layout_grid_v;
			int vp_width = vp_x_right - vp_x_left;
			int vp_height = vp_y_top - vp_y_bot;

			glViewport(vp_x_left, vp_y_bot, vp_width, vp_height);
			glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, vp_width, 0, vp_height, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glBegin(GL_QUADS);
				glColor3f(0.5f, 0.5f, 0.5f);
				glVertex2i(0, 0);
				glVertex2i(vp_width, 0);
				glVertex2i(vp_width, vp_height);
				glVertex2i(0, vp_height);
			glEnd();

			/* setup viewport for rendering (letter/pillarbox) */
			if (!video_data) {
				continue;
			}

			int video_data_width = video_data->data.plane[0].width;
			int video_data_height = video_data->data.plane[0].height;
			/* xxx: !params.aspect_ratio_lock, params.zoom_1to1 */
			int vp_x_centre = vp_width / 2;
			int vp_y_centre = vp_height / 2;
			int video_x_centre = video_data_width / 2;
			int video_y_centre = video_data_height / 2;

			float zoom = pow(2.f,(params.zoom-1.0f)/2.0f);
			/* determine bounding box to limit pan and scan action */
			int video_scaled_width = video_data_width * zoom;
			int video_scaled_height = video_data_height * zoom;
			int pan_x_limit = max(0, (video_scaled_width - vp_width) / 2);
			int pan_y_limit = max(0, (video_scaled_height - vp_height) / 2);
			params.pan_x = max(-pan_x_limit, min(pan_x_limit, params.pan_x));
			params.pan_y = max(-pan_y_limit, min(pan_y_limit, params.pan_y));

			glTranslatef(params.pan_x, -params.pan_y, 0.f);
			glTranslatef(vp_x_centre, vp_y_centre, 0.f);
			addStat(*stats, "Zoom", zoom * 100.0f, "%");
			glScalef(zoom, zoom, 1.0);
			glTranslatef(-video_x_centre, -video_y_centre, 0.f);

			//update the uniform variables in the fragment shader
			perfTimer.restart();
			int currentShader = chooseShader(video_data, params);
			updateShaderVars(programs[currentShader], video_data, colour_matrix);
			addStat(*stats, "UpdateVars", perfTimer.elapsed(), "ms");

			if (params.deinterlace) {
				glUseProgramObjectARB(programs[currentShader]);
				int i = glGetUniformLocationARB(programs[currentShader], "field");
				glUniform1fARB(i, (float)video_data->is_field1);
				glUseProgramObjectARB(0);
			}

			perfTimer.restart();
			::render(video_data, programs[currentShader]);
			addStat(*stats, "Render", perfTimer.elapsed(), "ms");

			unref_texture(video_data);
		}
	}

	/* draw a grid */
	if (grid_border > 0) {
		glViewport(0, 0, displaywidth, displayheight);
		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, displaywidth, 0, displayheight, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		for (int y = 0; y < layout_grid_v; y++) {
			for (int x = 0; x < layout_grid_h; x++) {
				int vp_x_left = x * displaywidth / layout_grid_h;
				int vp_x_right = (x+1) * displaywidth / layout_grid_h;
				int vp_y_bot = y * displayheight / layout_grid_v;
				int vp_y_top = (y+1) * displayheight / layout_grid_v;

				glBegin(GL_QUADS);
					glColor3f(0.f, 0.f, 0.f);
					if (y > 0) {
						glVertex2i(vp_x_left,  vp_y_bot - grid_border);
						glVertex2i(vp_x_right, vp_y_bot - grid_border);
						glVertex2i(vp_x_right, vp_y_bot + grid_border-1);
						glVertex2i(vp_x_left,  vp_y_bot + grid_border-1);
					}

					if (x > 0) {
						glVertex2i(vp_x_left - grid_border,   vp_y_bot);
						glVertex2i(vp_x_left + grid_border-1, vp_y_bot);
						glVertex2i(vp_x_left + grid_border-1, vp_y_top);
						glVertex2i(vp_x_left - grid_border,   vp_y_top);
					}
				glEnd();
			}
		}
	}

	glPopMatrix();
}

static int
chooseShader(VideoData *video_data, GLvideo_params& params)
{
	int current_shader;

	if (video_data->data.packing_format == V8P
	||  video_data->data.packing_format == V16P) {
		current_shader = shaderPlanar;
	}
	else
		current_shader = shaderUYVY;

	if (params.deinterlace)
		current_shader |= Deinterlace;
	else
		current_shader &= ~Deinterlace;

	return current_shader;
}

static void
updateShaderVars(int program, VideoData *video_data, float colour_matrix[4][4])
{
	if (DEBUG)
		printf("Updating fragment shader variables\n");

	float chroma_h_subsample;
	float chroma_v_subsample;
	switch (video_data->data.chroma_format) {
	case Cr420:
		chroma_h_subsample = 2;
		chroma_v_subsample = 2;
		break;
	case Cr422:
		chroma_h_subsample = 2;
		chroma_v_subsample = 1;
		break;
	default:
	case Cr444:
		chroma_h_subsample = 1;
		chroma_v_subsample = 1;
		break;
	}
	glUseProgramObjectARB(program);
	//data about the input file to the shader
	int i;
	i = glGetUniformLocationARB(program, "CHsubsample");
	glUniform1fARB(i, chroma_h_subsample);

	i = glGetUniformLocationARB(program, "CVsubsample");
	glUniform1fARB(i, chroma_v_subsample);

	i = glGetUniformLocationARB(program, "colour_matrix");
	glUniformMatrix4fvARB(i, 1, true, (float*) colour_matrix);

	glUseProgramObjectARB(0);
}
