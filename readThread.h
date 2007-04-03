#ifndef READTHREAD_RT_H_
#define READTHREAD_RT_H_

#include <QtGui>

class VideoRead;

class ReadThread : public QThread
{
public:
	ReadThread(VideoRead &vr);
	void run();
	void stop();
        
private:   	
	bool m_doReading;
	VideoRead &vr;	//parent
};

#endif /*GLVIDEO_RT_H_*/
