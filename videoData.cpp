#include "videoData.h"

#include <malloc.h>

VideoData::VideoData(int w, int h, DataFmt f)
	: format(f)
	, width(w)
	, height(h)
{
	switch(format) {
	
		case Planar444:
		case PlanarRGB:
		case RGB:
			dataSize = width * height * 3;
			break;
			
		case Planar422:
		case UYVY:
			dataSize = width * height * 2;
			break;
			
		case I420:
		case YV12:
			dataSize = (width * height * 3) / 2;
			break;
			
		case V216:
		case YV16:
			dataSize = width * height * 4;
			break;
			
		case V210:
			//3 10 bit components packed into 4 bytes
			dataSize = (width * height * 2 * 4) / 3;
			break;
			
		default:
			//oh-no!
			break;
	}
	
	//get some nicely page aligned memory
	data = (char *)valloc(dataSize);
	Ydata = data;
	
	switch(format) {
		//planar formats
		case Planar444:
		case PlanarRGB:
			Udata = Ydata + (width*height);
			Vdata = Udata + (width*height);
			break;
			
		case Planar422:
			Udata = Ydata + (width * height);
			Vdata = Ydata + ((width * height) / 2);
			break;
			
		case I420:
			Udata = Ydata + (width * height);
			Vdata = Udata + ((width * height) / 4);
			break;
			
		case YV12:
			Vdata = Ydata + (width * height);
			Udata = Vdata + ((width * height) / 4);
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
