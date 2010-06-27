#ifndef __GLVIDEO_RENDERER_H
#define __GLVIDEO_RENDERER_H

#include <GL/glew.h>

class VideoDataOld;

namespace GLVideoRenderer
{
class GLVideoRenderer {
public:
	virtual void createTextures(VideoDataOld *video_data) = 0;
	virtual void deleteTextures() = 0;
	virtual void uploadTextures(VideoDataOld *video_data) = 0;
	virtual void renderVideo(VideoDataOld *video_data, GLuint shader) = 0;
	virtual ~GLVideoRenderer() {};
};
}

#endif
