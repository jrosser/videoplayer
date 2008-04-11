#include "GLvideo_tradtex.h"
#include "videoData.h"

namespace GLVideoRenderer
{
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

	if (video_data->isPlanar) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat,
		             video_data->Cwidth, video_data->Cheight, 0,
		             video_data->glFormat, video_data->glType, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
		                video_data->glMinMaxFilter);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
		                video_data->glMinMaxFilter);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat,
		             video_data->Cwidth, video_data->Cheight, 0,
		             video_data->glFormat, video_data->glType, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
		                video_data->glMinMaxFilter);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
		                video_data->glMinMaxFilter);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, video_data->glInternalFormat,
	             video_data->glYTextureWidth, video_data->Yheight, 0,
	             video_data->glFormat, video_data->glType, NULL);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
	                video_data->glMinMaxFilter);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
	                video_data->glMinMaxFilter);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

void TradTex::uploadTextures(VideoData *video_data)
{
	if (video_data->isPlanar) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth,
		                video_data->Cheight, video_data->glFormat,
		                video_data->glType, video_data->Udata);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth,
		                video_data->Cheight, video_data->glFormat,
		                video_data->glType, video_data->Vdata);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0,
	                video_data->glYTextureWidth, video_data->Yheight,
	                video_data->glFormat, video_data->glType, video_data->Ydata);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

void TradTex::renderVideo(VideoData *video_data, GLuint shader_prog)
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgramObjectARB(shader_prog);
	int i;
	if (video_data->isPlanar) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
		i = glGetUniformLocationARB(shader_prog, "Utex");
		glUniform1iARB(i, 1); /* Bind Ytex to texture unit 1 */

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
		i = glGetUniformLocationARB(shader_prog, "Vtex");
		glUniform1iARB(i, 2); /* Bind Ytex to texture unit 2 */
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
	i = glGetUniformLocationARB(shader_prog, "Ytex");
	glUniform1iARB(i, 0); /* Bind Ytex to texture unit 0 */

	glDisable(GL_TEXTURE_RECTANGLE_ARB);

#if 0
	/* enable this to view the luma texture being rendered without
	 * going through the shader */
	glUseProgramObjectARB(0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
#endif

	/* NB, texture coordinates are relative to the texture data, (0,0)
	 * is the first texture pixel uploaded, since we use two different
	 * coordinate systems, the texture appears to be upside down */
	glBegin(GL_QUADS);
	glTexCoord2i(0, video_data->Yheight);
	glColor3f(1., 0., 0.);
	glVertex2i(0, 0);
	glTexCoord2i(video_data->Ywidth, video_data->Yheight);
	glColor3f(0., 1., 0.);
	glVertex2i(video_data->Ywidth, 0);
	glTexCoord2i(video_data->Ywidth, 0);
	glColor3f(0., 0., 1.);
	glVertex2i(video_data->Ywidth, video_data->Yheight);
	glTexCoord2i(0, 0);
	glColor3f(0., 0., 0.);
	glVertex2i(0, video_data->Yheight);
	glEnd();
	glUseProgramObjectARB(0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
}
} // namespace