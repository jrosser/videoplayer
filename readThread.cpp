#include <QtGui>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "readThread.h"
#include "videoRead.h"

#define DEBUG 0
#define LISTLEN 50

ReadThread::ReadThread(VideoRead &v) 
      : QThread(), vr(v)
{
	m_doReading = true;
}
    
void ReadThread::stop()
{
	m_doReading = false;
}
    
void ReadThread::run()
{
	printf("Starting readThread\n");

	int fd = 0;

	while(m_doReading) {

		//get the number of frames in the frameList
		QMutexLocker listLocker(&vr.listMutex);		
		int numFutureFrames = vr.futureFrames.size();
		int numPastFrames = vr.pastFrames.size();
		int currentFrame = vr.currentFrameNum;
		int firstFrame = vr.firstFrameNum;
		int lastFrame = vr.lastFrameNum;		
		listLocker.unlock();

		//printf("Futureframes=%d, PastFrames=%d\n", numFutureFrames, numPastFrames);
		
		//get the transport status
		QMutexLocker transportLocker(&vr.transportMutex);
		VideoRead::TransportControls ts = vr.transportStatus;
		transportLocker.unlock();

		if(fd <= 0) {
			printf("Trying to open file\n");
			fd = open(vr.fileName.toLatin1().data(), O_RDONLY | O_DIRECT);
			if(fd < 0) perror(NULL);					
		}
				
		if(fd > 0) {

			//add extra frames to the future frames list
			while(numFutureFrames < LISTLEN) {

				VideoData *newFrame = new VideoData(1920, 1080, VideoData::I420);
				int wantedFrame = currentFrame + numFutureFrames;
				wantedFrame %= lastFrame;

				off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;								
				off64_t sret = lseek64(fd, offset, SEEK_SET);
				if(!sret && offset != 0) perror("LSEEK");
												
				int rret = read(fd, newFrame->data, newFrame->dataSize);						
				if(!rret) perror("READ");
										
				listLocker.relock();
				vr.futureFrames.append(newFrame);
				numFutureFrames = vr.futureFrames.size();
				currentFrame = vr.currentFrameNum;
				listLocker.unlock();				
			}
			
			//add extra frames to the past frames list
			while(numPastFrames < LISTLEN) {
				
				VideoData *newFrame = new VideoData(1920, 1080, VideoData::I420);
				int wantedFrame = currentFrame - numPastFrames - 1;
				if(wantedFrame < 0) wantedFrame = (wantedFrame + lastFrame) + 1;
								
				off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;								
				off64_t sret = lseek64(fd, offset, SEEK_SET);
				if(!sret && offset != 0) perror("LSEEK");
												
				int rret = read(fd, newFrame->data, newFrame->dataSize);						
				if(!rret) perror("READ");
									
				listLocker.relock();
				vr.pastFrames.append(newFrame);
				numPastFrames = vr.pastFrames.size();
				currentFrame = vr.currentFrameNum;
				listLocker.unlock();				
			}
			
			//remove excess frames from the future list, which accumulate when playing backwards
			while(numFutureFrames > LISTLEN) {
				
				listLocker.relock();
				VideoData *oldFrame = vr.futureFrames.takeLast();
				numFutureFrames = vr.futureFrames.size();
				delete oldFrame;
				listLocker.unlock();				
			}
			
			//remove excess frames from the past list, which accumulate when playing forward					
			while(numPastFrames > LISTLEN) {
								
				listLocker.relock();
				VideoData *oldFrame = vr.pastFrames.takeLast();
				numPastFrames = vr.pastFrames.size();
				delete oldFrame;
				listLocker.unlock();							
			}
		}

		vr.frameConsumed.wait(&(vr.frameMutex));						
	}
}
