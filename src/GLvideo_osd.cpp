
#include <algorithm>

#include <GL/glew.h>

#include "freetype-gl/freetype-gl.h"
#include "freetype-gl/vertex-buffer.h"
#include "freetype-gl/utf8.h"

#include "GLvideo_osd.h"
#include "GLvideo_params.h"
#include "videoData.h"
#include "stats.h"

using namespace std;

#define DEBUG 0

static const rgba_t WHITE = {1., 1., 1., 1.};
static const rgba_t GREEN = {0., 1., 0., 1.};

typedef struct {
	float x, y, z;
	float s, t;
	float r, g, b, a;
} vertex_t;

static ivec4
prepare_string(vertex_buffer_t* buffer, markup_t* markup, vec2* pen, const char *str)
{
	utf8conv_state_t utf8state;
	init_utf8tounicode(&utf8state, str);

	ivec4 bbox = {0,0,0,0};

	unsigned previous = 0;
	while (unsigned current = utf8conv_getnext(&utf8state)) {
		texture_glyph_t *glyph = texture_font_get_glyph(markup->font, current);
		if (previous) {
			pen->x += texture_glyph_get_kerning(glyph, previous);
		} else {
			/* move origin so bounding box is tight around glyph */
			pen->x -= glyph->offset_x;
		}
		int x0  = pen->x + glyph->offset_x;
		int y0  = pen->y + glyph->offset_y;
		int x1  = x0 + glyph->width;
		int y1  = y0 - glyph->height;
		float s0 = glyph->s0;
		float t0 = glyph->t0;
		float s1 = glyph->s1;
		float t1 = glyph->t1;
		GLuint index = buffer->vertices->size;
		GLuint indices[] = {index, index+1, index+2,
		                    index, index+2, index+3};

		float r = markup->foreground_colour.r;
		float g = markup->foreground_colour.g;
		float b = markup->foreground_colour.b;
		float a = markup->foreground_colour.a;
		vertex_t vertices[] = { { x0,y0,0,  s0,t0,  r,g,b,a },
		                        { x0,y1,0,  s0,t1,  r,g,b,a },
		                        { x1,y1,0,  s1,t1,  r,g,b,a },
		                        { x1,y0,0,  s1,t0,  r,g,b,a } };
		vertex_buffer_push_back_indices( buffer, indices, 6 );
		vertex_buffer_push_back_vertices( buffer, vertices, 4 );

		/* bounding box */
		bbox.width = max(bbox.width, x1);
		bbox.height = max(bbox.height, y0);
		bbox.y = min(bbox.y, y1);
		bbox.x = min(bbox.x, x0);

		pen->x += glyph->advance_x;
		pen->y += glyph->advance_y;

		previous = current;
	}

	return bbox;
}

GLvideo_osd::GLvideo_osd(GLvideo_params &params)
{
	//const char *ttf = "./Vera.ttf";
	char *ttf = strdup(params.font_file.toLatin1().constData());
	font_atlas = texture_atlas_new( 512, 512, 1 );
	markup_normal.font = texture_font_new(font_atlas, ttf, params.osd_caption_ptsize);
	markup_stats.font = texture_font_new(font_atlas, ttf, 13);
	vertex_buffer = vertex_buffer_new( "v3f:t2f:c4f" );
	free(ttf);
	return;
}

GLvideo_osd::~GLvideo_osd()
{
}

void GLvideo_osd::render(unsigned viewport_width, unsigned viewport_height, VideoData *videoData, GLvideo_params &params)
{
	if (params.osd_bot != OSD_NONE)
		renderOSD(viewport_width, viewport_height, videoData, params);

	if (params.osd_perf)
		renderStats(viewport_width, viewport_height, params);
}

void GLvideo_osd::renderOSD(unsigned viewport_width, unsigned viewport_height, VideoData *videoData, GLvideo_params &params)
{
	//text string
	char str[255];
	switch (params.osd_bot) {
	case OSD_FRAMENUM:
		if (!videoData->is_interlaced) {
			sprintf(str, "%06ld", videoData->frame_number);
		} else {
			sprintf(str, "%06ld.%1d", videoData->frame_number, videoData->is_field1);
		}
		break;

	case OSD_CAPTION:
		if (params.caption.isEmpty())
			return;
		sprintf(str, "%s", params.caption.toLatin1().constData());
		break;

	default:
		return;
	}

	vertex_buffer_clear(vertex_buffer);

	vec2 pen = {0.,0.};
	markup_normal.foreground_colour = WHITE;
	ivec4 bbox = prepare_string(vertex_buffer, &markup_normal, &pen, str);

	//text box location, defaults to bottom left
	float tx = 0.05 * viewport_width;
	float ty = 0.05 * viewport_height;

	if (params.osd_bot==OSD_CAPTION) {
		//put the caption in the middle of the screen
		tx = ((int)viewport_width - (bbox.width)) / 2.0;
	}

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	bx1 = 0           - border;
	by1 = bbox.y      - border;
	bx2 = bbox.width  + border;
	by2 = bbox.height + border;

	//box beind text
	glPushMatrix();
	glTranslated(tx, ty, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0.0, 0.0, 0.0, params.osd_back_alpha);

	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();

	//text
	glColor4f(1.0, 1.0, 1.0, params.osd_text_alpha);
	glEnable(GL_TEXTURE_2D);
	vertex_buffer_render(vertex_buffer, GL_TRIANGLES, "vtc");
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glPopMatrix();
}

static ivec4
mergeBox(ivec4 a, ivec4 b)
{
	ivec4 r;
	r.x = min(a.x, b.x);
	r.y = min(a.y, b.y);
	r.width = max(a.width, b.width);
	r.height = max(a.height, b.height);
	return r;
}

void GLvideo_osd::renderStats(unsigned viewport_width, unsigned viewport_height, GLvideo_params &params)
{
	vertex_buffer_clear(vertex_buffer);
	ivec4 total_bbox = {0,0,0,0};
	vec2 pen = {0.,0.};

	//render the stats text
	int new_key_width = 0;
	Stats &s = Stats::getInstance();
	s.mutex.lock();
	for (Stats::const_iterator it = s.begin();
	     it != s.end();
	     it++)
	{
		markup_stats.foreground_colour = GREEN;
		ivec4 bbox = prepare_string(vertex_buffer, &markup_stats, &pen, (*it).first.c_str());
		total_bbox = mergeBox(total_bbox, bbox);

		pen.y -= markup_stats.font->height - markup_stats.font->linegap;
		pen.x = 0;
		markup_stats.foreground_colour = WHITE;

		QMutexLocker(&(*it).second->mutex);
		for (Stats::Section::const_iterator it2 = (*it).second->begin();
		     it2 != (*it).second->end();
		     it2++)
		{
			bbox = prepare_string(vertex_buffer, &markup_stats, &pen, (*it2).first.c_str());
			total_bbox = mergeBox(total_bbox, bbox);
			new_key_width = max(new_key_width, bbox.width);

			/* gap between columns */
			pen.x = text_val_start_pos;

			bbox = prepare_string(vertex_buffer, &markup_stats, &pen, (*it2).second.c_str());
			total_bbox = mergeBox(total_bbox, bbox);

			pen.y -= markup_stats.font->height - markup_stats.font->linegap;
			pen.x = 0;
		}
	}
	s.mutex.unlock();

	text_val_start_pos = new_key_width * 1.1;

	glPushMatrix();
	glTranslated(0.025 * viewport_width, (1 - 0.025) * viewport_height, 0); //near the top left corner

	//box beind text
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0.0, 0.0, 0.0, 0.7f);

	// black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1 = 0                 - border;
	float by1 = total_bbox.y      - border;
	float bx2 = total_bbox.width  + border;
	float by2 = total_bbox.height + border;
	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();

	glColor4f(1.0, 1.0, 1.0, params.osd_text_alpha);
	glEnable(GL_TEXTURE_2D);
	vertex_buffer_render(vertex_buffer, GL_TRIANGLES, "vtc");
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glPopMatrix();
}
