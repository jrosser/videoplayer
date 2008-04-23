#include "GLvideo_osd.h"
#include "GLvideo_params.h"
#include "GLvideo_perf.h"
#include "videoData.h"

#define DEBUG 0

GLvideo_osd::~GLvideo_osd()
{
	if(font)
		delete font;
}

void GLvideo_osd::render(VideoData *videoData, GLvideo_params &params, GLvideo_perf &perf)
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
		renderOSD(videoData, params, perf);

		if(params.osd_perf)
			renderPerf(videoData, perf);
	}
}

void GLvideo_osd::renderOSD(VideoData *videoData, GLvideo_params &params, GLvideo_perf &perf)
{
	//bounding box of text
	float cx1, cy1, cz1, cx2, cy2, cz2;

	//text string
	char str[255];
	switch (params.osd_bot) {
	case OSD_FRAMENUM:
		sprintf(str, "%06ld", videoData->frameNum);
		font->BBox("000000", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_FPS:
		sprintf(str, "%06ld, fps:%3.2f", videoData->frameNum, perf.renderRate);
		font->BBox("000000, fps:000.00", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_INT:
		sprintf(str, "%06ld, %s:%s", videoData->frameNum,
		        params.interlaced_source ? "Int" : "Prog",
		        params.deinterlace ? "On" : "Off");
		font->BBox("000000, Prog:Off", cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	case OSD_CAPTION:
		sprintf(str, "%s", params.caption.toLatin1().constData());
		font->BBox(str, cx1, cy1, cz1, cx2, cy2, cz2);
		break;

	default:
		str[0] = '\0';
	}

	//text box location, defaults to bottom left
	float tx=0.05 * videoData->Ywidth;
	float ty=0.05 * videoData->Yheight;

	if (params.osd_bot==4) {
		//put the caption in the middle of the screen
		tx = videoData->Ywidth - ((cx2 - cx1) * params.osd_scale);
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
	font->Render(str);
	glDisable(GL_POLYGON_SMOOTH);
	glPopMatrix();
}

void GLvideo_osd::drawText(char *str)
{
	glEnable(GL_POLYGON_SMOOTH);
	glPushMatrix();
	font->Render(str);
	glPopMatrix();
	glDisable(GL_POLYGON_SMOOTH);
}

void GLvideo_osd::draw2Text(const char *str1, const char *str2, float h_spacing)
{
	glEnable(GL_POLYGON_SMOOTH);
	glPushMatrix();

	glPushMatrix();
	font->Render(str1);
	glPopMatrix();

	glTranslatef(h_spacing, 0, 0);

	glPushMatrix();
	font->Render(str2);
	glPopMatrix();

	glPopMatrix();
	glDisable(GL_POLYGON_SMOOTH);
}

void GLvideo_osd::drawPerfTimer(const char *str, int num, const char *units, float h_spacing)
{
	char str2[255];
	sprintf(str2, "%d%s", num, units);
	draw2Text(str, str2, h_spacing);
}

void GLvideo_osd::renderPerf(VideoData *videoData, GLvideo_perf &perf)
{
	float tx = 0.05 * videoData->Ywidth;
	float ty = 0.95 * videoData->Yheight;
	char str[255];
	float cx1, cy1, cz1, cx2, cy2, cz2;

	font->BBox("0", cx1, cy1, cz1, cx2, cy2, cz2);
	float spacing = (cy2-cy1) * 1.5;
	float width = cx2 - cx1;

	//black box that text is rendered onto, larger than the text by 'border'
	float border = 10;
	float bx1, by1, bx2, by2;
	bx1 = -border;
	by1 = spacing;
	bx2 = width * 25;
	by2 = (spacing * -15) - border*2;

	glPushMatrix();
	glTranslated(tx, ty, 0); //near the top left corner
	glScalef(0.25, 0.25, 0); //reduced in size compared to OSD

	//box beind text
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 0.0, 0.7);

	glBegin(GL_QUADS);
	glVertex2f(bx1, by1);
	glVertex2f(bx1, by2);
	glVertex2f(bx2, by2);
	glVertex2f(bx2, by1);
	glEnd();

	glColor4f(0.0, 1.0, 0.0, 0.5);

	sprintf(str, "Rendering");
	drawText(str);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	int gap = width * 17;
	
	glTranslated(0, -spacing, 0);
	drawPerfTimer("ReadData", perf.readData, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("ConvertFormat", perf.convertFormat, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("UpdateVars", perf.updateVars, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RepeatWait", perf.repeatWait, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Upload", perf.upload, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RenderVideo", perf.renderVideo, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("RenderOSD", perf.renderOSD, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("SwapBuffers", perf.swapBuffers, "ms", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Interval", perf.interval, "ms", gap);

	glColor4f(0.0, 1.0, 0.0, 0.5);

	glTranslated(0, -spacing, 0);
	glTranslated(0, -spacing, 0);
	sprintf(str, "I/O");
	drawText(str);

	glColor4f(1.0, 1.0, 1.0, 0.5);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Future Queue", perf.futureQueue, "", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("Past Queue", perf.pastQueue, "", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("I/O Thread Load", perf.IOLoad, "%", gap);

	glTranslated(0, -spacing, 0);
	drawPerfTimer("ReadRate", perf.IOBandwidth, "MB/s", gap);

	glPopMatrix();
}
