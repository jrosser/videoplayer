#include "videoData.h"

#include <malloc.h>

	int Ywidth;		//luminance dimensions
	int Yheight;
	
	int Cwidth;		//chrominance dimensions
	int Cheight;
	
	int YdataSize;	//number of bytes for each component 
	int UdataSize;
	int VdataSize;
		
	char *data;
	char *Ydata;
	char *Udata;
	char *Vdata;        

VideoData::VideoData(int w, int h, DataFmt f)
	: format(f)
	, Ywidth(w)
	, Yheight(h)
{
	switch(format) {
	
		case Planar444:
		case RGB:
			Cwidth  = Ywidth;
			Cheight = Yheight;
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight;
			VdataSize = Ywidth * Yheight;						
			dataSize  = Ywidth * Yheight * 3;
			break;
			
		case Planar422:
		case UYVY:
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;		
			YdataSize = Ywidth * Yheight;
			UdataSize = Ywidth * Yheight / 2;
			VdataSize = Ywidth * Yheight / 2;								
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
			YdataSize = 0;
			UdataSize = 0;
			VdataSize = 0;
			dataSize = Ywidth * Yheight * 4;
			break;
			
		case V210:
			//3 10 bit components packed into 4 bytes
			Cwidth  = Ywidth / 2;
			Cheight = Yheight;			
			YdataSize = 0;
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
			isPlanar = true;
			Udata = Ydata + YdataSize;
			Vdata = Udata + UdataSize;
			break;
						
		case YV12:
			isPlanar = true;
			//U&V components are exchanged
			Vdata = Ydata + (Ywidth * Yheight);
			Udata = Vdata + ((Ywidth * Yheight) / 4);
			break;
			
		case RGB:
		case UYVY:
		case V216:
		case YV16:	
		case V210:
			//all muxed formats
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
