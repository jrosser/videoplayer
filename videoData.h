#ifndef VIDEODATA_H_
#define VIDEODATA_H_

class VideoData
{
public:

	enum DataFmt { Planar444, Planar422, I420, YV12, Planar411, UYVY, V216, YV16, V210, RGB };	//how is the data packed?

	VideoData(int width, int height, DataFmt f);
	~VideoData();
	
	unsigned long frameNum;
	
	DataFmt    format;
	bool 	   isPlanar;
	
	int Ywidth;		//luminance dimensions
	int Yheight;
	
	int Cwidth;		//chrominance dimensions
	int Cheight;
	
	int dataSize;
	int YdataSize;	//number of bytes for each component 
	int UdataSize;
	int VdataSize;
		
	char *data;
	char *Ydata;
	char *Udata;
	char *Vdata;        
};

#endif
