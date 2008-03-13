#define __GLVIDEO_PBOTEX

#include <cassert>

#include "GLfuncs.h"
#include "GLvideo_renderer.h"

#include "videoData.h"

/* NB, any changes to the oublic interface
 * /must/ be reflected in GLvideo_tradtex.h */
namespace GLVideoRenderer {
class PboTex : public GLVideoRenderer {
public:
	virtual void createTextures(VideoData *video_data);
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

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
PboTex::~PboTex()
{
}

void PboTex::createTextures(VideoData *video_data)
{
    int i=0;

    glGenTextures(3, (GLuint *)&textures);
    GLfuncs::glGenBuffers(1, &buf);

    /* The explicit bind to the zero pixel unpack buffer object allows
     * passing NULL in glTexImage2d() to be unspecified texture data
     * (ie, create the storage for the texture but don't upload antything
     */
    GLfuncs::glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    if(video_data->isPlanar) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,textures.u);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat, video_data->Cwidth , video_data->Cheight, 0, video_data->glFormat, video_data->glType, NULL);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, video_data->glMinMaxFilter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, video_data->glMinMaxFilter);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,textures.v);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat, video_data->Cwidth , video_data->Cheight, 0, video_data->glFormat, video_data->glType, NULL);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, video_data->glMinMaxFilter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, video_data->glMinMaxFilter);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    }

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat, video_data->glYTextureWidth, video_data->Yheight, 0, video_data->glFormat, video_data->glType, NULL);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, video_data->glMinMaxFilter);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, video_data->glMinMaxFilter);
    
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

void PboTex::uploadTextures(VideoData *video_data)
{
    void *ioMem;
    
    GLfuncs::glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, buf);
    
    if(video_data->isPlanar) {
        GLfuncs::glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->UdataSize, NULL, GL_STREAM_DRAW);
        ioMem = GLfuncs::glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
        assert(ioMem);
        memcpy(ioMem, video_data->Udata, video_data->UdataSize);
        GLfuncs::glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth, video_data->Cheight, video_data->glFormat, video_data->glType, BUFFER_OFFSET(0));

        GLfuncs::glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->VdataSize, NULL, GL_STREAM_DRAW);
        ioMem = GLfuncs::glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
        assert(ioMem);
        memcpy(ioMem, video_data->Vdata, video_data->VdataSize);
        GLfuncs::glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth, video_data->Cheight, video_data->glFormat, video_data->glType, BUFFER_OFFSET(0));
    }

    GLfuncs::glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->YdataSize, NULL, GL_STREAM_DRAW);
    ioMem = GLfuncs::glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
    assert(ioMem);
    memcpy(ioMem, video_data->Ydata, video_data->YdataSize);
    GLfuncs::glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Ywidth, video_data->Yheight, video_data->glFormat, video_data->glType, BUFFER_OFFSET(0));

    GLfuncs::glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

void PboTex::renderVideo(VideoData *video_data, GLuint shader_prog)
{
    glClear(GL_COLOR_BUFFER_BIT);

    GLfuncs::glUseProgramObjectARB(shader_prog);
    int i;
    if (video_data->isPlanar) {
    	GLfuncs::glActiveTexture(GL_TEXTURE1);
    	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
    	i = GLfuncs::glGetUniformLocationARB(shader_prog, "Utex");
    	GLfuncs::glUniform1iARB(i, 1);  /* Bind Ytex to texture unit 1 */
    
    	GLfuncs::glActiveTexture(GL_TEXTURE2);
    	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
    	i = GLfuncs::glGetUniformLocationARB(shader_prog, "Vtex");
    	GLfuncs::glUniform1iARB(i, 2);  /* Bind Ytex to texture unit 2 */
    }

    GLfuncs::glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
    i = GLfuncs::glGetUniformLocationARB(shader_prog, "Ytex");
    GLfuncs::glUniform1iARB(i, 0);  /* Bind Ytex to texture unit 0 */

    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glBegin(GL_QUADS);
        glTexCoord2i(0,0);
	glColor3f(1., 0., 0.);
        glVertex2i(0,0);
        glTexCoord2i(video_data->Ywidth,0);
	glColor3f(0., 1., 0.);
        glVertex2i(video_data->Ywidth,0);
        glTexCoord2i(video_data->Ywidth, video_data->Yheight);
	glColor3f(0., 0., 1.);
        glVertex2i(video_data->Ywidth, video_data->Yheight);
        glTexCoord2i(0, video_data->Yheight);
	glColor3f(0., 0., 0.);
        glVertex2i(0, video_data->Yheight);
    glEnd();
    GLfuncs::glUseProgramObjectARB(0);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
}
} // namespace
