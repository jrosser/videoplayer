#include "videoData.h"

#include <malloc.h>


VideoData::VideoData(int w, int h, DataFmt f)
	: format(f)
	, Ywidth(w)
	, Yheight(h)
{
	switch(format) {
	
		case Planar444:
			Cwidth  = Ywidth;
			Cheight = Yheight;
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight;
			VdataSize = Ywidth * Yheight;						
			dataSize  = Ywidth * Yheight * 3;
			break;
			
		case Planar422:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight / 2;
			VdataSize = Ywidth * Yheight / 2;								
			dataSize = Ywidth * Yheight * 2;
			break;
					
		case UYVY:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight * 2;
			UdataSize = 0;
			VdataSize = 0;								
			dataSize = Ywidth * Yheight * 2;
			break;

		case Planar411:
			Cwidth  = Ywidth / 4;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight / 4;
			VdataSize = Ywidth * Yheight / 4;								
			dataSize = (Ywidth * Yheight * 3) / 2;
			break;
								
		case I420:
		case YV12:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight / 2;		
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight / 4;
			VdataSize = Ywidth * Yheight / 4;								
			dataSize = (Ywidth * Yheight * 3) / 2;
			break;
			
		case V216:
		case YV16:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;	
			YdataSize = Ywidth * Yheight * 4;
			UdataSize = 0;
			VdataSize = 0;
			dataSize = Ywidth * Yheight * 4;
			break;
			
		case V210:
			//3 10 bit components packed into 4 bytes
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;			
			YdataSize = (Ywidth * Yheight * 2 * 4) / 3;
			UdataSize = 0;
			VdataSize = 0;			
			dataSize = (Ywidth * Yheight * 2 * 4) / 3;
			break;
			
		default:
			//oh-no!
			break;
	}
	
	//get some nicely page aligned memory 
	data = (char *)valloc(dataSize);
	Ydata = data;
	isPlanar = false;
	
	switch(format) {
		//planar formats
		case Planar444:
		case Planar422:
		case Planar411:
		case I420:
			glYTextureWidth = Ywidth;		
			glInternalFormat = GL_LUMINANCE;
			glFormat = GL_LUMINANCE;
			glType = GL_UNSIGNED_BYTE;
			isPlanar = true;
			Udata = Ydata + YdataSize;
			Vdata = Udata + UdataSize;
			break;
						
		case YV12:
			isPlanar = true;
			glYTextureWidth = Ywidth;			
			glInternalFormat = GL_LUMINANCE;
			glFormat = GL_LUMINANCE;
			glType = GL_UNSIGNED_BYTE;			
			//U&V components are exchanged
			Vdata = Ydata + (Ywidth * Yheight);
			Udata = Vdata + ((Ywidth * Yheight) / 4);
			break;
			
		case UYVY:
			//muxed 8 bit format
			glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad			
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;			
			break;
					
		case V210:
			//really hard!
			//muxed 10 bit format - this is wrong, FIXME
			glYTextureWidth = Ywidth;
			glInternalFormat = GL_LUMINANCE;
			glFormat = GL_LUMINANCE;
			glType = GL_UNSIGNED_BYTE;			
			Udata = Ydata;
			Vdata = Ydata;			
			break;
			
		case YV16:	
		case V216:
			glYTextureWidth = Ywidth / 2;	//2 samples per YUV quad
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_SHORT;			
			Udata = Ydata;
			Vdata = Ydata;
			break;
			
		default:
			//oh-no!
			break;
	}			
}
	
VideoData::~VideoData()
{
	if(data)
		free(data);	
};
