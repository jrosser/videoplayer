#ifndef __GLVIDEO_PBOTEX
#define __GLVIDEO_PBOTEX

#include "GLvideo_renderer.h"

namespace GLVideoRenderer {
class PboTex : public GLVideoRenderer {
public:
	virtual void createTextures(VideoData *video_data);
	virtual void uploadTextures(VideoData *video_data);
	virtual void renderVideo(VideoData *video_data, GLuint shader);
	virtual ~PboTex();
};
}

#endif

