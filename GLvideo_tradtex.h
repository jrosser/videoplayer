#ifndef __GLVIDEO_TRADTEX
#define __GLVIDEO_TRADTEX

#include "GLvideo_renderer.h"

namespace GLVideoRenderer {
class TradTex : public GLVideoRenderer {
public:
	virtual void createTextures(VideoData *video_data);
	virtual void uploadTextures(VideoData *video_data);
	virtual void renderVideo(VideoData *video_data, GLuint shader);
	virtual ~TradTex();
};
}

#endif

