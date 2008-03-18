#include "GLfuncs.h"
#include "GLvideo_tradtex.h"

#include "videoData.h"

namespace GLVideoRenderer {
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
TradTex::~TradTex()
{
}

void TradTex::createTextures(VideoData *video_data)
{
#if 0
    if (textures != 0)
	    glDeleteTexture ...
#endif
    glGenTextures(3, (GLuint *)&textures);

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

void TradTex::uploadTextures(VideoData *video_data)
{
    if(video_data->isPlanar) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth, video_data->Cheight, video_data->glFormat, video_data->glType, video_data->Udata);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth, video_data->Cheight, video_data->glFormat, video_data->glType, video_data->Vdata);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->glYTextureWidth, video_data->Yheight, video_data->glFormat, video_data->glType, video_data->Ydata);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

void TradTex::renderVideo(VideoData *video_data, GLuint shader_prog)
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

#if 0
	GLfuncs::glUseProgramObjectARB(0);
    i = GL_REPLACE; glTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &i); 
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
#endif
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
