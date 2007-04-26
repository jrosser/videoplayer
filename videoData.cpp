#include "videoData.h"

#include <malloc.h>

#ifdef Q_OS_WIN32
#define valloc malloc
#endif


VideoData::VideoData(int w, int h, DataFmt f)
	: diskFormat(f)
	, Ywidth(w)
	, Yheight(h)
{
	switch(diskFormat) {
	
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
	glMinMaxFilter = GL_LINEAR;	//linear interpolation
	renderFormat = diskFormat;
	
	switch(diskFormat) {
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
			glYTextureWidth = (Ywidth * 2 * 4) / (3 * 4);			
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;			
			glType = GL_UNSIGNED_INT;
			glMinMaxFilter = GL_NEAREST;			
			Udata = Ydata;
			Vdata = Ydata;			
			break;
			
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

//utility function to convert V210 data into something easier to display
//it is too difficult to do this in the openGL shader
void VideoData::convertV210()
{	
	int v210LineLength = (2 * Ywidth * 4) / 3;	//there are three components in 4 bytes
 
   	int *uyvyLine = (int*)data;		//do the conversion in place     	      		
	int *word = (int *)data;
		
    for (int y=0; y<Yheight; y++) {
     
  		register int src;
   		register int dest;
   		
   		for(int x=0; x<v210LineLength/16; x++) {

			//NASTY! this is not endian safe... it deals with four bytes at once in an int. Yuk :-(
			src   = *word++;   			   	   			
   			dest  = (src >> 2) & 0xFF;		   //Cb
   			dest |= (src >> 4) & 0xFF00;	   //Y
   			dest |= (src >> 6) & 0xFF0000;	   //Cr      			
   			src   = *word++;   			
   			dest |= (src << 22) & 0xFF000000;  //Y  			
			*uyvyLine++ = dest;

   			dest  = (src >> 12) & 0xFF;		   //Cb
   			dest |= (src >> 14) & 0xFF00;	   //Y
   			src   = *word++;   			
   			dest |= (src << 14) & 0xFF0000;    //Cr
   			dest |= (src << 12) & 0xFF000000;  //Y   			
   			*uyvyLine++ = dest;
   			
			dest  = (src >> 22) & 0xFF;	       //Cb			
			src   = *word++;			
			dest |= (src << 6) & 0xFF00;       //Y
			dest |= (src << 4) & 0xFF0000;     //Cr
			dest |= (src << 2) & 0xFF000000;   //Y   			
   			*uyvyLine++ = dest;   			   			   			
   		}
	}
	
	//change the description of the data to match UYVY
	//note that the dataSize
	renderFormat = UYVY;
	glYTextureWidth = Ywidth / 2;	//2 Y samples per RGBA quad			
	glInternalFormat = GL_RGBA;
	glFormat = GL_RGBA;
	glType = GL_UNSIGNED_BYTE;	
	YdataSize = Ywidth * Yheight * 2;
	UdataSize = 0;
	VdataSize = 0;													
}
