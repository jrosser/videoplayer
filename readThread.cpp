#include <QtGui>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "readThread.h"
#include "videoRead.h"

#define DEBUG 1
#define LISTLEN 10

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
    int lastSpeed = 1;
    
	while(m_doReading) {

		//get the number of frames in the frameList
		QMutexLocker listLocker(&vr.listMutex);		
		int numFutureFrames = vr.futureFrames.size();
		int numPastFrames = vr.pastFrames.size();
		int currentFrame = vr.currentFrameNum;
		int firstFrame = vr.firstFrameNum;
		int lastFrame = vr.lastFrameNum;		
		listLocker.unlock();
		
		//get the transport status
		QMutexLocker transportLocker(&vr.transportMutex);
		VideoRead::TransportControls ts = vr.transportStatus;
		int speed = vr.transportSpeed;
		transportLocker.unlock();

		if(fd <= 0) {
			if(DEBUG) printf("Opening file\n");
			fd = open(vr.fileName.toLatin1().data(), O_RDONLY | O_DIRECT);
			
			if(fd < 0) { 
				printf("[%s]\n", vr.fileName.toLatin1().data());
				perror("OPEN");
			}
		}


		 
		 				
		if(fd > 0) {

			//trash the contents of the frame lists when we change speed
			//this could be made MUCH cleverer - to keep past and future frames that we need when changing speed
			if(speed != lastSpeed) {
				
				printf("Trashing frame lists - ts=%d speed=%d, last=%d\n", ts, speed, lastSpeed);
				
				listLocker.relock();
				
				while(vr.futureFrames.size()) {
					VideoData *frame = vr.futureFrames.takeLast();
					delete frame;	
				}

				while(vr.pastFrames.size()) {
					VideoData *frame = vr.pastFrames.takeLast();
					delete frame;	
				}
								
				listLocker.unlock();
				
			}

			lastSpeed = speed;
			
			//add extra frames to the future frames list
			while(numFutureFrames < LISTLEN) {
				
				int lastFutureFrame = currentFrame - 1;								//the default reading position when we have no frames
				listLocker.relock();
				if(vr.futureFrames.size() > 0)
					lastFutureFrame = vr.futureFrames.last()->frameNum;				//get the number of the last future frame
				listLocker.unlock();
				
				int wantedFrame = lastFutureFrame + speed;			//work out the number of the next future frame				
				wantedFrame %= lastFrame + 1;						//wrap from the end of the file to the start
								
				//if(DEBUG) printf("Getting future frame %d\n", wantedFrame);
				VideoData *newFrame = new VideoData(1920, 1080, VideoData::I420);	//allocate for a new frame
				newFrame->frameNum = wantedFrame;												

				off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;	//seek to the wanted frame								
				off64_t sret = lseek64(fd, offset, SEEK_SET);
				if(!sret && offset != 0) perror("LSEEK");
												
				int rret = read(fd, newFrame->data, newFrame->dataSize);						
				if(!rret) perror("READ");
										
				listLocker.relock();
				vr.futureFrames.append(newFrame);
				numFutureFrames = vr.futureFrames.size();
				listLocker.unlock();				
			}
			
			//add extra frames to the past frames list
			while(numPastFrames < LISTLEN) {
								
				int lastPastFrame = currentFrame;								//the default last frame when we have no frames in the list				
				listLocker.relock();
				if(vr.pastFrames.size() > 0)
					lastPastFrame = vr.pastFrames.last()->frameNum;				//get the number of the last future frame
				listLocker.unlock();
																
				int wantedFrame = lastPastFrame - speed;
				if(wantedFrame < 0) wantedFrame += (lastFrame + 1);
				wantedFrame %= lastFrame + 1;
				
				//if(DEBUG) printf("Getting past frame %d\n", wantedFrame);
												
				VideoData *newFrame = new VideoData(1920, 1080, VideoData::I420);				
				newFrame->frameNum = wantedFrame;
								
				off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;								
				off64_t sret = lseek64(fd, offset, SEEK_SET);
				if(!sret && offset != 0) perror("LSEEK");
																				
				int rret = read(fd, newFrame->data, newFrame->dataSize);						
				if(!rret) perror("READ");
									
				listLocker.relock();
				vr.pastFrames.append(newFrame);
				numPastFrames = vr.pastFrames.size();
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
