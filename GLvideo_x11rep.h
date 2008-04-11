#ifndef __GLVIDEO_X11REPEATER_H
#define __GLVIDEO_X11REPEATER_H

#include "GLvideo_repeater.h"

class VideoData;

namespace GLVideoRepeater
{
class X11Repeater : public GLVideoRepeater {
public:
	virtual void createRGBDestination(VideoData *video_data);
	virtual void setupRGBDestination();
	virtual void setupScreenDestination();
	virtual unsigned int repeat(unsigned int);
	virtual void renderVideo(VideoData *video_data);
	virtual ~X11Repeater();
};
}

#endif
