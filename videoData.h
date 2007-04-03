#ifndef VIDEODATA_H_
#define VIDEODATA_H_

class VideoData
{
public:

	enum DataFmt { Planar444, Planar422, I420, YV12, PlanarRGB, UYVY, V216, YV16, V210, RGB };

	VideoData(int width, int height, DataFmt f);
	~VideoData();
	
	DataFmt format;
	
	int  width;
	int  height;	
	int  dataSize;
	char *data;
	char *Ydata;
	char *Udata;
	char *Vdata;        
};

#endif
