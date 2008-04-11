#include <string.h>
#include <cassert>

//needed for NULL
#include <stdio.h>

#include "GLvideo_pbotex.h"

#include "videoData.h"

namespace GLVideoRenderer
{
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
PboTex::~PboTex()
{
}

void PboTex::createTextures(VideoData *video_data)
{
	glGenTextures(3, (GLuint *)&textures);
	glGenBuffers(1, &buf);

	/* The explicit bind to the zero pixel unpack buffer object allows
	 * passing NULL in glTexImage2d() to be unspecified texture data
	 * (ie, create the storage for the texture but don't upload antything
	 */
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

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

void PboTex::uploadTextures(VideoData *video_data)
{
	void *ioMem;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, buf);

	if (video_data->isPlanar) {
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->UdataSize, NULL,
		             GL_STREAM_DRAW);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		assert(ioMem);
		memcpy(ioMem, video_data->Udata, video_data->UdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.u);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth,
		                video_data->Cheight, video_data->glFormat,
		                video_data->glType, BUFFER_OFFSET(0));

		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->VdataSize, NULL,
		             GL_STREAM_DRAW);
		ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		assert(ioMem);
		memcpy(ioMem, video_data->Vdata, video_data->VdataSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.v);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Cwidth,
		                video_data->Cheight, video_data->glFormat,
		                video_data->glType, BUFFER_OFFSET(0));
	}

	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, video_data->YdataSize, NULL,
	             GL_STREAM_DRAW);
	ioMem = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
	assert(ioMem);
	memcpy(ioMem, video_data->Ydata, video_data->YdataSize);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.y);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, video_data->Ywidth,
	                video_data->Yheight, video_data->glFormat,
	                video_data->glType, BUFFER_OFFSET(0));

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

void PboTex::renderVideo(VideoData *video_data, GLuint shader_prog)
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
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0);
	glColor3f(1., 0., 0.);
	glVertex2i(0, 0);
	glTexCoord2i(video_data->Ywidth, 0);
	glColor3f(0., 1., 0.);
	glVertex2i(video_data->Ywidth, 0);
	glTexCoord2i(video_data->Ywidth, video_data->Yheight);
	glColor3f(0., 0., 1.);
	glVertex2i(video_data->Ywidth, video_data->Yheight);
	glTexCoord2i(0, video_data->Yheight);
	glColor3f(0., 0., 0.);
	glVertex2i(0, video_data->Yheight);
	glEnd();
	glUseProgramObjectARB(0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
}
} // namespace
