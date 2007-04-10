#ifndef READTHREAD_RT_H_
#define READTHREAD_RT_H_

#include <QtGui>

#include "videoData.h"

class VideoRead;

class ReadThread : public QThread
{
public:
	ReadThread(VideoRead &vr);
	void run();
	void stop();
        
private:   	
	void addFutureFrames();
	void addPastFrames();	

	bool m_doReading;
	VideoRead &vr;	//parent
	
	//info about the lists of frames		
	int numFutureFrames;
	int numPastFrames;
		
	//the extents of the sequence
	int firstFrame;
	int lastFrame;
	
	//playback speed
	int speed;
	int lastSpeed;
	
	//information about the video data	
	VideoData::DataFmt dataFormat;
	int videoWidth;
	int videoHeight;
	
	//video file
	int fd;
		
};

#endif
