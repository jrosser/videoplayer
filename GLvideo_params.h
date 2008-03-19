#ifndef __GLVIDEO_PARAMS_H
#define __GLVIDEO_PARAMS_H

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
    float luminance_offset1;
    float chrominance_offset1;
    float luminance_mul;
    float chrominance_mul;
    float luminance_offset2;
    float chrominance_offset2;

	/* set the video levels to computer (0-255)
	 * or video (16-235) */
	bool matrix_scaling;

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
	bool aspect_ratio_lock;
};

#endif
