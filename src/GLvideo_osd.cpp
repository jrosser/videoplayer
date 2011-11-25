
#include <GL/glew.h>
#include <FTGL/ftgl.h>

#include "GLvideo_osd.h"
#include "GLvideo_params.h"
#include "videoData.h"
#include "stats.h"

#define DEBUG 0

GLvideo_osd::GLvideo_osd()
	: font(0)
{
	return;
}

GLvideo_osd::~GLvideo_osd()
{
	if(font)
		delete font;
}

void GLvideo_osd::render(VideoData *videoData, GLvideo_params &params)
{
	if (!params.osd_valid) {
		if (DEBUG)
			printf("Changing font\n");
		if (font)
			delete font;

		font = new FTGLPolygonFont(params.font_file.toLatin1().constData());
		if (!font->Error()) {
			font->FaceSize(72);
			font->CharMap(ft_encoding_unicode);
		}
		else {
			delete font;
			font = NULL;
		}
		
		params.osd_valid = true;
	}

	if(font) {

		if(params.osd_bot != OSD_NONE)
			renderOSD(videoData, params);

		if(params.osd_perf)
			renderStats(videoData);
	}
}

void GLvideo_osd::renderOSD(VideoData *videoData, GLvideo_params &params)
{
	//bounding box of text
	float cx1, cy1, cz1, cx2, cy2, cz2;

	//text string
	char str[255];
	switch (params.osd_bot) {
	case OSD_FRAMENUM:
		if (!videoData->is_interlaced) {
			sprintf(str, "%06ld", videoData->frame_number);
			font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2);
		} else {
			sprintf(str, "%06ld.%1d", videoData->frame_number, videoData->is_field1);
			font->BBox("000000.0", cx1, cy1, cz1, cx2, cy2, cz2);
		}
		break;

	case OSD_CAPTION:
		if (params.caption.isEmpty())
			return;
		sprintf(str, "%s", params.caption.toLatin1().constData());
		font->BBox(str, cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	default:
		return;
	}

	//text box location, defaults to bottom left
	float tx=0.05 * videoData->data.plane[0].width;
	float ty=0.05 * videoData->data.plane[0].height;

	if (params.osd_bot==OSD_CAPTION) {
		//put the caption in the middle of the screen
		tx = videoData->data.plane[0].width - ((cx2 - cx1) * params.osd_scale);
		tx/=2;
	}

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	bx1 = cx1 - border;
	by1 = cy1 - border;
	bx2 = cx2 + border;
	by2 = cy2 + border;

	//box beind text
	glPushMatrix();
	glTranslated(tx, ty, 0);
	glScalef(params.osd_scale, params.osd_scale, 0); //scale the on screen display

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
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_BLEND);
	font->Render(str);
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_BLEND);
	glPopMatrix();
}

void GLvideo_osd::drawText(const char *str)
{
	glPushMatrix();
	font->Render(str);
	glPopMatrix();
}

void GLvideo_osd::draw2Text(const char *str1, const char *str2, float h_spacing)
{
	glPushMatrix();

	glPushMatrix();
	font->Render(str1);
	glPopMatrix();

	glTranslatef(h_spacing, 0, 0);

	glPushMatrix();
	font->Render(str2);
	glPopMatrix();

	glPopMatrix();
}

void GLvideo_osd::drawPerfTimer(const char *str, int num, const char *units, float h_spacing)
{
	char str2[255];
	sprintf(str2, "%d%s", num, units);
	draw2Text(str, str2, h_spacing);
}

void GLvideo_osd::renderStats(VideoData *videoData)
{
	//position of the stats relative to the video
	float tx = 0.05 * videoData->data.plane[0].width;
	float ty = 0.95 * videoData->data.plane[0].height;

	//determine approx character size
	float cx1, cy1, cz1, cx2, cy2, cz2;

	font->BBox("0", cx1, cy1, cz1, cx2, cy2, cz2);
	float spacing = (cy2-cy1) * 1.5;
	float width = cx2 - cx1;
	float gap = width * 17;

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	static int n_lines;	//remember how many lines of stats there were last time
	bx1 = -border;
	by1 = spacing;
	bx2 = width * 25;
	by2 = (spacing * n_lines * -1) - border*2;

	glPushMatrix();
	glTranslated(tx, ty, 0); //near the top left corner
	glScalef(0.25, 0.25, 0); //reduced in size compared to OSD

	//box beind text
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glColor4f(0.0, 0.0, 0.0, 0.7f);

	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();
	
	//render the stats text
	Stats &s = Stats::getInstance();
	s.mutex.lock();
	n_lines = 0;
	for (Stats::const_iterator it = s.begin();
	     it != s.end();
	     it++)
	{
		glColor4f(0.0, 1.0, 0.0, 0.5);
		drawText((*it).first.c_str());
		glTranslated(0, -spacing, 0);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		n_lines++;

		QMutexLocker(&(*it).second->mutex);
		for (Stats::Section::const_iterator it2 = (*it).second->begin();
		     it2 != (*it).second->end();
		     it2++)
		{
			draw2Text((*it2).first.c_str(), (*it2).second.c_str(), gap);
			glTranslated(0, -spacing, 0);
			n_lines++;
		}
	}
	s.mutex.unlock();

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);
	glPopMatrix();
}
