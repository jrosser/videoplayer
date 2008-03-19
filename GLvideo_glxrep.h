#ifndef __GLVIDEO_GLXREPEATER_H
#define __GLVIDEO_GLXREPEATER_H

#include "GLvideo_repeater.h"

class VideoData;

namespace GLVideoRepeater {
class GlxRepeater : public GLVideoRepeater {
public:
	virtual void createRGBDestination(VideoData *video_data);
	virtual void setupRGBDestination();
	virtual void setupScreenDestination();
	virtual unsigned int repeat(unsigned int);	
	virtual void renderVideo(VideoData *video_data);
	virtual ~GlxRepeater();
};
}

#endif
