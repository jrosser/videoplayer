#ifndef __GLVIDEO_RENDERER_H
#define __GLVIDEO_RENDERER_H

#include <GL/glew.h>

class VideoData;

namespace GLVideoRenderer
{
class GLVideoRenderer {
public:
	virtual void createTextures(VideoData *video_data) = 0;
	virtual void uploadTextures(VideoData *video_data) = 0;
	virtual void renderVideo(VideoData *video_data, GLuint shader) = 0;
	virtual ~GLVideoRenderer()
	{
	}
	;
};
}

#endif
