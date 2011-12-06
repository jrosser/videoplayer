#ifndef __GLVIDEO_PBOTEX
#define __GLVIDEO_PBOTEX

#include <GL/glew.h>
#include "GLvideo_renderer.h"
class VideoData;

/* NB, any changes to the public interface
 * /must/ be reflected in GLvideo_tradtex.h */
namespace GLVideoRenderer
{
class PboTex : public GLVideoRenderer {
public:
	PboTex();
	virtual void createTextures(VideoData *video_data);
	virtual void deleteTextures();
	virtual void uploadTextures(VideoData *video_data);
	virtual void renderVideo(VideoData *video_data, GLuint shader);
	virtual ~PboTex();

private:
	/* handles for the y/u/v/ textures */
	struct {
		GLuint y;
		GLuint u;
		GLuint v;
	} textures;
	GLuint buf;
};
}

#endif

