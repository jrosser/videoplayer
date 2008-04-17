#ifndef __GLVIDEO_PARAMS_H
#define __GLVIDEO_PARAMS_H

#include <QtCore>

enum OSDmode {
	OSD_NONE = 0,
	OSD_FRAMENUM,
	OSD_FPS,
	OSD_INT,
	OSD_CAPTION,
	OSD_MAX,
};

inline
OSDmode& operator++(OSDmode& l)
{
	l = (OSDmode) ((l + 1) % OSD_MAX);
	return l;
}

inline
std::istream& operator>>(std::istream& s, OSDmode& m)
{
	s >> (unsigned int &)m;
	return s;
}

struct GLvideo_params {
	int frame_repeats;

	bool osd_valid;
	QString font_file;
	QString caption;
	float osd_scale;
	OSDmode osd_bot;
	bool osd_perf;
	float osd_back_alpha;
	float osd_text_alpha;

	/* offset1 is the offset applied to src video
	 * offset1 result is multiplied by mul
	 * offset2 is then added */
	bool matrix_valid;
	int input_luma_range;
	int input_luma_blacklevel;
	int input_chroma_blacklevel;
	int output_range;
	int output_blacklevel;
	float luminance_mul;
	float chrominance_mul;
	float luminance_offset2;
	float chrominance_offset2;

	/*optionally turn off luminance or chrominance channels*/
	bool show_luma;
	bool show_chroma;

	/* what colour matrix */
	float matrix_Kr;
	float matrix_Kg;
	float matrix_Kb;

	/* @interlaced_source@ modifies behaviour when repeating
	 * frames (paused) and playing backwards (field reversal)
	 * @deinterlace@ causes a .5 .5 deinterlacer to be used */
	bool interlaced_source;
	bool deinterlace;

	/* @aspect_ratio_lock@ forces the assumption that
	 * the video PAR is the same as the device PAR */
	bool view_valid;
	bool aspect_ratio_lock;
};

/* SetLumaCoeffsRec709
 * Set Kr,Kg,Kb to have the luma coefficients defined in ITU-R Rec BT.709
 * NB, these are *not* the colour primaries */
static inline void
SetLumaCoeffsRec709(GLvideo_params &p)
{
	p.matrix_Kr = 0.2126;
	p.matrix_Kg = 0.7152;
	p.matrix_Kb = 0.0722;
}

/* SetLumaCoeffsRec601
 * Set Kr,Kg,Kb to have the luma coefficients defined in ITU-R Rec BT.601
 * NB, these are *not* the colour primaries */
static inline void
SetLumaCoeffsRec601(GLvideo_params &p)
{
	p.matrix_Kr = 0.299;
	p.matrix_Kg = 0.587;
	p.matrix_Kb = 0.114;
}
#endif
