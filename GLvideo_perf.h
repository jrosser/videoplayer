#ifndef __GLVIDEO_PERF_H
#define __GLVIDEO_PERF_H

struct GLvideo_perf {

	int getParams;
	int readData;
	int convertFormat;
	int updateVars;
	int repeatWait;
	int upload;
	int renderVideo;
	int renderOSD;
	int swapBuffers;
	int interval;

	//I/O performance
	int futureQueue;
	int pastQueue;
	int IOLoad;
	int IOBandwidth;
	
	float renderRate;
};

#endif
