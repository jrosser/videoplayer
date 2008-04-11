#ifndef __GLVIDEO_TRADTEX
#define __GLVIDEO_TRADTEX

#include "GLvideo_renderer.h"
class VideoData;

/* NB, any changes to the oublic interface
 * /must/ be reflected in GLvideo_tradtex.h */
namespace GLVideoRenderer
{
class TradTex : public GLVideoRenderer {
public:
	virtual void createTextures(VideoData *video_data);
	virtual void uploadTextures(VideoData *video_data);
	virtual void renderVideo(VideoData *video_data, GLuint shader);
	virtual ~TradTex();

private:
	/* handles for the y/u/v/ textures */
	struct {
		GLuint y;
		GLuint u;
		GLuint v;
	} textures;
};

}

#endif

