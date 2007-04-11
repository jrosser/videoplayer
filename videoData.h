#ifndef VIDEODATA_H_
#define VIDEODATA_H_

#include <GL/gl.h>

#include "GLvideo_rt.h"

class VideoData
{
public:

	enum DataFmt { Planar444, Planar422, I420, YV12, Planar411, UYVY, V216, YV16, V210 };	//how is the data packed?
	
	VideoData(int width, int height, DataFmt f);
	~VideoData();
	
	unsigned long frameNum;
	
	DataFmt    format;		//enumerated format
	bool 	   isPlanar;	//flag to indicate that we have a seperate texture for each component to copy to the GPU
	
	GLenum		glInternalFormat;	//format used to describe which components of the texture are applied to the texels
	GLenum		glFormat;			//format of the texture data, GL_LUMINANCE or GL_RGBA in these applications
	GLenum		glType;				//the data type for the texture, GL_UNSIGNED_BYTE, GL_INT etc...

	int 		glYTextureWidth;	//the width of the Y texture, which may actually be UYVY data packed as RGBA. There
									//will be half the number of RGBA quads as Y pixels across the image as each quad
									//stores two luminance samples	
		
	int Ywidth;			//luminance dimensions in pixels
	int Yheight;
	
	int Cwidth;			//chrominance dimensions in pixels
	int Cheight;
	
	int dataSize;		//total allocation at *data
	int YdataSize;		//number of bytes for each component 
	int UdataSize;
	int VdataSize;
		
	char *data;			//pointer to allocated block
	char *Ydata;		//pointer to luminance, or multiplexed YCbCr
	char *Udata;		//pointers to planar chrominance components, if format is planar
	char *Vdata;        
};

#endif
