#ifndef __GLVIDEO_REPEATER_H
#define __GLVIDEO_REPEATER_H

class VideoData;

namespace GLVideoRepeater
{
class GLVideoRepeater {
public:
	virtual void createRGBDestination(VideoData *video_data) = 0;
	virtual void setupRGBDestination() = 0;
	virtual void setupScreenDestination() = 0;
	virtual unsigned int repeat(unsigned int) = 0;
	virtual void renderVideo(VideoData *video_data) = 0;
	virtual ~GLVideoRepeater()
	{
	}
	;
};
}

#endif
